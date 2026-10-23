#include "Tab.hpp"

#include "CategoryTabsApi.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "Icon.hpp"
#include "Panel.hpp"
#include "logger.hpp"

void Tab::Register() {
    CategoryTabsTab tab{
        "HidePacks",
        "Hide Packs",
        Icons::Eye.data(),
        Icons::Eye.size(),
        [](void* navigation) { Panel::Show(static_cast<GlobalNamespace::LevelFilteringNavigationController*>(navigation)); },
        [] { Panel::SetListVisible(true); },
    };
    if (!CategoryTabsApi::Register(tab)) {
        Logger.error("CategoryTabs is not installed or is too old, the Hide Packs tab will not appear");
    }
}
