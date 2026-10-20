#include "autohooks/shared/hooks.hpp"
#include "logger.hpp"

#include <algorithm>
#include <cstring>

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "Icon.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "bsml/shared/BSML-Lite.hpp"

using namespace GlobalNamespace;

namespace {
    using Category = SelectLevelCategoryViewController::LevelCategory;
    using Info = SelectLevelCategoryViewController::LevelCategoryInfo;

    constexpr int GameTabs = 4;
    constexpr float GameWidth = 30.0f;
    constexpr float TabWidth = 7.5f;

    int ourCategory = -1;
    int enabledTabs = 0;

    int FindFreeValue(ArrayW<Info*> infos) {
        int highest = 0;
        for (auto info : infos) {
            if (info) {
                highest = std::max(highest, static_cast<int>(info->levelCategory.value__));
            }
        }
        return highest + 1;
    }

    bool HasInfo(ArrayW<Info*> infos) {
        for (auto info : infos) {
            if (info && info->levelCategory.value__ == ourCategory) {
                return true;
            }
        }
        return false;
    }

    bool HasCategory(ArrayW<Category> categories) {
        for (auto category : categories) {
            if (category.value__ == ourCategory) {
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
        if (ourCategory < 0) {
            ourCategory = FindFreeValue(infos);
            Logger.info("Using category {}", ourCategory);
        }
        if (HasInfo(infos)) {
            return;
        }
        auto info = Info::New_ctor();
        info->levelCategory = ourCategory;
        info->localizedKey = "";
        info->categoryIcon = LoadIcon();
        ArrayW<Info*> extended(infos.size() + 1);
        infos.copy_to(extended, 0);
        extended[infos.size()] = info;
        self->_allLevelCategoryInfos = extended;
    }

    ArrayW<Category> AddCategory(ArrayW<Category> categories) {
        if (ourCategory < 0 || HasCategory(categories)) {
            return categories;
        }
        ArrayW<Category> extended(categories.size() + 1);
        categories.copy_to(extended, 0);
        extended[categories.size()] = ourCategory;
        return extended;
    }

    void Widen(SelectLevelCategoryViewController* self) {
        if (enabledTabs <= GameTabs) {
            return;
        }
        float wanted = GameWidth + static_cast<float>(enabledTabs - GameTabs) * TabWidth;
        auto rect = self->get_rectTransform();
        auto size = rect->sizeDelta;
        if (size.x < wanted) {
            rect->sizeDelta = {wanted, size.y};
            Logger.info("Category bar width {} for {} tabs", wanted, enabledTabs);
        }
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
        enabledTabs = enabled.size();
    } catch (std::exception const& e) {
        Logger.warn("Could not add the category: {}", e.what());
    }
    SelectLevelCategoryViewController_Setup(self, selected, enabled);
}

MAKE_LATE_HOOK_MATCH(
    SelectLevelCategoryViewController_DidActivate,
    &SelectLevelCategoryViewController::DidActivate,
    void,
    SelectLevelCategoryViewController* self,
    bool firstActivation,
    bool addedToHierarchy,
    bool screenSystemEnabling
) {
    SelectLevelCategoryViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    try {
        Widen(self);
    } catch (std::exception const& e) {
        Logger.warn("Could not widen the bar: {}", e.what());
    }
}

MAKE_LATE_HOOK_MATCH(
    LevelFilteringNavigationController_UpdateSecondChildControllerContent,
    &LevelFilteringNavigationController::UpdateSecondChildControllerContent,
    void,
    LevelFilteringNavigationController* self,
    Category category
) {
    if (ourCategory >= 0 && category.value__ == ourCategory) {
        Logger.info("Our tab was selected");
        return;
    }
    LevelFilteringNavigationController_UpdateSecondChildControllerContent(self, category);
}
