#include "autohooks/shared/hooks.hpp"
#include "logger.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "Icon.hpp"
#include "bsml/shared/BSML-Lite.hpp"

using namespace GlobalNamespace;

namespace {
    using Category = SelectLevelCategoryViewController::LevelCategory;
    using Info = SelectLevelCategoryViewController::LevelCategoryInfo;

    constexpr int Wanted = 11;
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
            info->localizedKey = "";
            info->categoryIcon = LoadIcon();
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
        Logger.warn("Could not add the category: {}", e.what());
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
        Logger.info("Our tab was selected");
        return;
    }
    LevelFilteringNavigationController_UpdateSecondChildControllerContent(self, category);
}
