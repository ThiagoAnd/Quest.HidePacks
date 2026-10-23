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
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Texture2D.hpp"
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
        return BSML::Lite::ArrayToSprite(bytes);
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

    void Survey(ArrayW<Info*> infos) {
        static bool done = false;
        if (done) {
            return;
        }
        done = true;
        try {
            for (auto info : infos) {
                if (info) {
                    Describe(info->categoryIcon, "category " + std::to_string(info->levelCategory.value__));
                }
            }
            for (auto sprite : UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>()) {
                if (!sprite) {
                    continue;
                }
                std::string name = Text(sprite->name);
                std::string lower = name;
                std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
                if (lower.find("eye") != std::string::npos || lower.find("hide") != std::string::npos || lower.find("hidden") != std::string::npos || lower.find("visib") != std::string::npos || lower.find("unseen") != std::string::npos) {
                    Logger.info("ICON candidate {}", name);
                }
            }
        } catch (std::exception const& e) {
            ReportError("icon survey", e);
        }
    }

    void AddInfo(SelectLevelCategoryViewController* self) {
        auto infos = self->_allLevelCategoryInfos;
        if (!infos) {
            return;
        }
        Survey(infos);
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
