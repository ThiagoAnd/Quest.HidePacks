#pragma once

#include <string>
#include <vector>

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "bsml/shared/BSML/Components/Settings/ToggleSetting.hpp"

namespace PackState {
    struct Entry {
        std::string id;
        std::string name;
        bool dlc = true;
        UnityW<UnityEngine::Sprite> cover;
    };

    std::vector<Entry> const& Entries();
    void Load(GlobalNamespace::LevelFilteringNavigationController* navigation);
    void BeginSession();
    void MarkBuilt();
    bool RowsStale();
    bool IsVisible(std::string const& id);
    void AddToggle(BSML::ToggleSetting* toggle);
    void AddRow(UnityEngine::Transform* row);
    void BindCounter(TMPro::TextMeshProUGUI* text);
    void BindApply(UnityEngine::UI::Button* button);
    void OnSwitch(std::string const& id, bool visible);
    void Refresh();
    void Reset();
    void ShowAll();
    void Apply();
    void FilterRows(bool onlyDlc);
    void Tick();
}
