#include "autohooks/shared/hooks.hpp"
#include "logger.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "Icon.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/FilterMode.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "UnityEngine/TextureWrapMode.hpp"
#include "Panel.hpp"
#include "bsml/shared/BSML-Lite.hpp"

using namespace GlobalNamespace;

namespace {
    using Category = SelectLevelCategoryViewController::LevelCategory;
    using Info = SelectLevelCategoryViewController::LevelCategoryInfo;

    constexpr int Wanted = 1;
    std::vector<int> ours;

    int FindFreeValue(ArrayW<Info*> infos) {
        int highest = 0;
        for (auto info : infos) {
            if (info) {
                highest = std::max(highest, static_cast<int>(info->levelCategory.value__));
            }
        }
        return highest + 1;
    }

    bool IsOurs(int value) {
        return std::find(ours.begin(), ours.end(), value) != ours.end();
    }

    bool HasInfo(ArrayW<Info*> infos, int value) {
        for (auto info : infos) {
            if (info && info->levelCategory.value__ == value) {
                return true;
            }
        }
        return false;
    }

    bool HasCategory(ArrayW<Category> categories, int value) {
        for (auto category : categories) {
            if (category.value__ == value) {
                return true;
            }
        }
        return false;
    }

    UnityEngine::Sprite* LoadIcon() {
        ArrayW<uint8_t> bytes(Icons::Eye.size());
        std::memcpy(bytes.begin(), Icons::Eye.data(), Icons::Eye.size());
        auto plain = BSML::Lite::ArrayToSprite(bytes);
        if (!plain || !plain->texture) {
            return plain;
        }
        auto source = plain->texture;
        auto texture = UnityEngine::Texture2D::New_ctor(source->width, source->height, UnityEngine::TextureFormat::RGBA32, true, false);
        texture->SetPixels32(source->GetPixels32());
        texture->Apply(true, false);
        texture->filterMode = UnityEngine::FilterMode::Trilinear;
        texture->wrapMode = UnityEngine::TextureWrapMode::Clamp;
        auto sprite = BSML::Lite::TextureToSprite(texture);
        return sprite ? sprite : plain;
    }

    void Describe(UnityEngine::Sprite* sprite, std::string const& label) {
        if (!sprite) {
            Logger.info("ICON {} has no sprite", label);
            return;
        }
        auto texture = sprite->texture;
        if (!texture) {
            Logger.info("ICON {} sprite {} has no texture", label, Text(sprite->name));
            return;
        }
        Logger.info("ICON {} sprite {} texture {}x{} mips {} filter {} aniso {} ppu {} rect {}x{}", label, Text(sprite->name), texture->width, texture->height, texture->mipmapCount, static_cast<int>(texture->filterMode), texture->anisoLevel, sprite->pixelsPerUnit, sprite->rect.width, sprite->rect.height);
    }

    void AddInfo(SelectLevelCategoryViewController* self) {
        auto infos = self->_allLevelCategoryInfos;
        if (!infos) {
            return;
        }
        if (ours.empty()) {
            int first = FindFreeValue(infos);
            for (int i = 0; i < Wanted; i++) {
                ours.push_back(first + i);
            }
            Logger.info("Using categories {} to {}", first, first + Wanted - 1);
        }
        for (int value : ours) {
            if (HasInfo(infos, value)) {
                continue;
            }
            auto info = Info::New_ctor();
            info->levelCategory = value;
            info->localizedKey = "Hide Packs";
            info->categoryIcon = LoadIcon();
            Describe(info->categoryIcon, "ours");
            ArrayW<Info*> extended(infos.size() + 1);
            infos.copy_to(extended, 0);
            extended[infos.size()] = info;
            infos = extended;
        }
        self->_allLevelCategoryInfos = infos;
    }

    ArrayW<Category> AddCategory(ArrayW<Category> categories) {
        for (int value : ours) {
            if (HasCategory(categories, value)) {
                continue;
            }
            ArrayW<Category> extended(categories.size() + 1);
            categories.copy_to(extended, 0);
            extended[categories.size()] = value;
            categories = extended;
        }
        return categories;
    }
}

MAKE_LATE_HOOK_MATCH(
    SelectLevelCategoryViewController_Setup,
    &SelectLevelCategoryViewController::Setup,
    void,
    SelectLevelCategoryViewController* self,
    Category selected,
    ArrayW<Category> enabled
) {
    try {
        AddInfo(self);
        enabled = AddCategory(enabled);
    } catch (std::exception const& e) {
        ReportError("could not add the category", e);
    }
    SelectLevelCategoryViewController_Setup(self, selected, enabled);
}

MAKE_LATE_HOOK_MATCH(
    LevelFilteringNavigationController_UpdateSecondChildControllerContent,
    &LevelFilteringNavigationController::UpdateSecondChildControllerContent,
    void,
    LevelFilteringNavigationController* self,
    Category category
) {
    if (IsOurs(category.value__)) {
        try {
            Panel::Show(self);
        } catch (std::exception const& e) {
            ReportError("could not show the panel", e);
        }
        if (self->_annotatedBeatmapLevelCollectionsViewController) {
            self->_annotatedBeatmapLevelCollectionsViewController->gameObject->SetActive(false);
        }
        return;
    }
    Panel::SetListVisible(true);
    LevelFilteringNavigationController_UpdateSecondChildControllerContent(self, category);
    if (self->_annotatedBeatmapLevelCollectionsViewController) {
        self->_annotatedBeatmapLevelCollectionsViewController->gameObject->SetActive(category.value__ <= 2);
    }
}
