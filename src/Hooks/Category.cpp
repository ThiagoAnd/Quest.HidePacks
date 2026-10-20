#include "autohooks/shared/hooks.hpp"
#include "logger.hpp"

#include <cstring>

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "Icon.hpp"
#include "bsml/shared/BSML-Lite.hpp"

using namespace GlobalNamespace;

namespace {
    using Category = SelectLevelCategoryViewController::LevelCategory;
    using Info = SelectLevelCategoryViewController::LevelCategoryInfo;

    constexpr int OurCategory = 7;

    bool HasInfo(ArrayW<Info*> infos) {
        for (auto info : infos) {
            if (info && info->levelCategory.value__ == OurCategory) {
                return true;
            }
        }
        return false;
    }

    bool HasCategory(ArrayW<Category> categories) {
        for (auto category : categories) {
            if (category.value__ == OurCategory) {
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
        if (!infos || HasInfo(infos)) {
            return;
        }
        auto info = Info::New_ctor();
        info->levelCategory = OurCategory;
        info->localizedKey = "";
        info->categoryIcon = LoadIcon();
        ArrayW<Info*> extended(infos.size() + 1);
        infos.copy_to(extended, 0);
        extended[infos.size()] = info;
        self->_allLevelCategoryInfos = extended;
    }

    ArrayW<Category> AddCategory(ArrayW<Category> categories) {
        if (HasCategory(categories)) {
            return categories;
        }
        ArrayW<Category> extended(categories.size() + 1);
        categories.copy_to(extended, 0);
        extended[categories.size()] = OurCategory;
        return extended;
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
    if (category.value__ == OurCategory) {
        Logger.info("Our tab was selected");
        return;
    }
    LevelFilteringNavigationController_UpdateSecondChildControllerContent(self, category);
}
