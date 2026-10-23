#include "Tab.hpp"

#include "CategoryTabsApi.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "Icon.hpp"
#include "Panel.hpp"
#include "logger.hpp"

namespace {
    bool registered = false;
}

bool Tab::Registered() {
    return registered;
}

void Tab::Register() {
    CategoryTabsTab tab{
        "HidePacks",
        "Hide Packs",
        Icons::Eye.data(),
        Icons::Eye.size(),
        [](void* navigation) { Panel::Show(static_cast<GlobalNamespace::LevelFilteringNavigationController*>(navigation)); },
        [] { Panel::SetListVisible(true); },
    };
    registered = CategoryTabsApi::Register(tab);
    if (!registered) {
        Logger.error("CategoryTabs is not installed or is too old, the Hide Packs tab will not appear and no pack will be hidden");
    }
}
