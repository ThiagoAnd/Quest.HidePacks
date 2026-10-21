#include "Panel.hpp"

#include <array>
#include <string>
#include <vector>

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "HMUI/Touchable.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/Helpers/creation.hpp"
#include "logger.hpp"

DEFINE_TYPE(HidePacks, PanelViewController);

namespace {
    constexpr float PanelWidth = 90.0f;
    constexpr float PanelHeight = 62.0f;
    constexpr float ListHeight = 40.0f;

    constexpr std::array<char const*, 14> SampleNames = {
        "Original Soundtrack Vol. 1", "Original Soundtrack Vol. 2", "Original Soundtrack Vol. 3",
        "Original Soundtrack Vol. 4", "Original Soundtrack Vol. 5", "Original Soundtrack Vol. 6",
        "Original Soundtrack Vol. 7", "Extras", "Camellia Music Pack", "Monstercat Music Pack Vol. 1",
        "Rocket League x Monstercat", "Green Day Music Pack", "Timbaland Music Pack", "BTS Music Pack"
    };

    UnityEngine::UI::LayoutElement* Sized(UnityEngine::Transform* child, UnityEngine::Transform* root, float height) {
        UnityEngine::Transform* top = child;
        while (top->parent && top->parent.ptr() != root) {
            top = top->parent.ptr();
        }
        auto element = top->gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!element) {
            element = top->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        }
        element->preferredHeight = height;
        element->minHeight = height;
        return element;
    }

    UnityEngine::UI::HorizontalLayoutGroup* Row(UnityEngine::Transform* parent, float height) {
        auto row = BSML::Lite::CreateHorizontalLayoutGroup(parent);
        row->spacing = 2.0f;
        row->childAlignment = UnityEngine::TextAnchor::MiddleCenter;
        row->childControlWidth = true;
        row->childControlHeight = true;
        row->childForceExpandWidth = false;
        row->childForceExpandHeight = false;
        auto element = row->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        element->preferredHeight = height;
        element->minHeight = height;
        return row;
    }
}

void HidePacks::PanelViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemDisabling) {
    if (!firstActivation) {
        return;
    }
    gameObject->AddComponent<HMUI::Touchable*>();

    auto root = BSML::Lite::CreateVerticalLayoutGroup(transform);
    auto rootRect = root->GetComponent<UnityEngine::RectTransform*>();
    rootRect->anchorMin = {0.5f, 0.5f};
    rootRect->anchorMax = {0.5f, 0.5f};
    rootRect->pivot = {0.5f, 0.5f};
    rootRect->sizeDelta = {PanelWidth, PanelHeight};
    rootRect->anchoredPosition = {0.0f, 0.0f};
    root->spacing = 1.5f;
    root->childAlignment = UnityEngine::TextAnchor::UpperCenter;
    root->childControlWidth = true;
    root->childControlHeight = true;
    root->childForceExpandWidth = true;
    root->childForceExpandHeight = false;

    auto header = Row(root->transform, 8.0f);
    BSML::Lite::CreateUIButton(header->transform, "All", [] { Logger.info("All"); });
    BSML::Lite::CreateUIButton(header->transform, "None", [] { Logger.info("None"); });
    BSML::Lite::CreateUIButton(header->transform, "Invert", [] { Logger.info("Invert"); });

    auto list = BSML::Lite::CreateScrollableSettingsContainer(root->transform);
    Sized(list->transform, root->transform, ListHeight);
    for (auto name : SampleNames) {
        BSML::Lite::CreateToggle(list->transform, name, true, [name](bool value) {
            Logger.info("{} {}", name, value);
        });
    }

    auto footer = Row(root->transform, 8.0f);
    BSML::Lite::CreateText(footer->transform, "0 of 14 hidden", 3.5f, {0.0f, 0.0f}, {40.0f, 6.0f});
    BSML::Lite::CreateUIButton(footer->transform, "Cancel", [] { Logger.info("Cancel"); });
    BSML::Lite::CreateUIButton(footer->transform, "Apply", [] { Logger.info("Apply"); });
}

namespace Panel {
    void Show(GlobalNamespace::LevelFilteringNavigationController* navigation) {
        static UnityW<HidePacks::PanelViewController> panel;
        if (!panel) {
            panel = BSML::Helpers::CreateViewController<HidePacks::PanelViewController*>();
        }
        navigation->ReplaceSecondViewController(panel.ptr());
    }
}
