#pragma once

#include "HMUI/ViewController.hpp"
#include "custom-types/shared/macros.hpp"

DECLARE_CLASS_CODEGEN(HidePacks, PanelViewController, HMUI::ViewController) {
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool addedToHierarchy, bool screenSystemDisabling);
    DECLARE_INSTANCE_METHOD(void, Update);
};

namespace GlobalNamespace {
    class LevelFilteringNavigationController;
}

namespace Panel {
    void SetListVisible(bool visible);
    void Show(GlobalNamespace::LevelFilteringNavigationController* navigation);
}
