#include "Panel.hpp"

#include <array>
#include <string>
#include <vector>

#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "HMUI/Touchable.hpp"
#include "UnityEngine/CanvasGroup.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
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
    constexpr float PanelWidth = 80.0f;
    constexpr float PanelHeight = 62.0f;
    constexpr float ListHeight = 40.0f;
    constexpr float SlotWidth = 95.0f;
    constexpr float SlotHeight = 10.0f;
    constexpr float SlotCenter = 17.0f;
    constexpr float PanelTop = 12.0f;

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
    rootRect->anchorMin = {0.5f, 1.0f};
    rootRect->anchorMax = {0.5f, 1.0f};
    rootRect->pivot = {0.5f, 1.0f};
    rootRect->sizeDelta = {PanelWidth, PanelHeight};
    rootRect->anchoredPosition = {-SlotCenter, -PanelTop};
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
        auto toggle = BSML::Lite::CreateToggle(list->transform, name, true, [name](bool value) {
            Logger.info("{} {}", name, value);
        });
        auto element = toggle->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!element) {
            element = toggle->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        }
        element->minWidth = PanelWidth - 14.0f;
        element->preferredWidth = PanelWidth - 14.0f;
        element->flexibleWidth = 0.0f;
    }

    auto footer = Row(root->transform, 8.0f);
    BSML::Lite::CreateText(footer->transform, "0 of 14 hidden", 3.5f, {0.0f, 0.0f}, {40.0f, 6.0f});
    BSML::Lite::CreateUIButton(footer->transform, "Cancel", [] { Logger.info("Cancel"); });
    BSML::Lite::CreateUIButton(footer->transform, "Apply", [] { Logger.info("Apply"); });
}

namespace Panel {
    void SetListVisible(bool visible) {
      try {
        static UnityW<UnityEngine::CanvasGroup> group;
        if (!group) {
            auto all = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionViewController*>();
            for (auto navigation : all) {
                if (navigation && navigation->gameObject->activeInHierarchy) {
                    auto object = navigation->gameObject;
                    group = object->GetComponent<UnityEngine::CanvasGroup*>();
                    if (!group) {
                        group = object->AddComponent<UnityEngine::CanvasGroup*>();
                    }
                    break;
                }
            }
        }
        if (!group) {
            Logger.warn("List controller not found");
            return;
        }
        group->alpha = visible ? 1.0f : 0.0f;
        group->blocksRaycasts = visible;
        group->interactable = visible;
      } catch (...) {
        Logger.warn("SetListVisible failed");
      }
    }

    void Show(GlobalNamespace::LevelFilteringNavigationController* navigation) {
        static UnityW<HidePacks::PanelViewController> panel;
        if (!panel) {
            panel = BSML::Helpers::CreateViewController<HidePacks::PanelViewController*>();
            auto rect = panel->GetComponent<UnityEngine::RectTransform*>();
            rect->anchorMin = {0.5f, 0.5f};
            rect->anchorMax = {0.5f, 0.5f};
            rect->pivot = {0.5f, 0.5f};
            rect->sizeDelta = {SlotWidth, SlotHeight};
        }
        navigation->ReplaceSecondViewController(panel.ptr());
        SetListVisible(false);
    }
}
