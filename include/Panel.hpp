#pragma once

#include "HMUI/ViewController.hpp"
#include "custom-types/shared/macros.hpp"

DECLARE_CLASS_CODEGEN(HidePacks, PanelViewController, HMUI::ViewController) {
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool addedToHierarchy, bool screenSystemDisabling);
};

namespace GlobalNamespace {
    class LevelFilteringNavigationController;
}

namespace Panel {
    void Dump(int category);
    void SetListVisible(bool visible);
    void Show(GlobalNamespace::LevelFilteringNavigationController* navigation);
}
