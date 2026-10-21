#include "Panel.hpp"

#include <array>
#include <string>
#include <vector>

#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/Touchable.hpp"
#include "UnityEngine/CanvasGroup.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "UnityEngine/UI/Image.hpp"
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
    constexpr float CoverSize = 6.0f;
    constexpr float ButtonHeight = 8.0f;
    constexpr float HeaderLift = -0.6f;

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

    UnityEngine::UI::Button* MakeButton(UnityEngine::Transform* parent, char const* text, std::string_view style, std::function<void()> onClick) {
        auto button = BSML::Lite::CreateUIButton(parent, text, style, {0.0f, 0.0f}, {0.0f, 0.0f}, std::move(onClick));
        if (!button) {
            return button;
        }
        auto element = button->gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!element) {
            element = button->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        }
        element->minHeight = ButtonHeight;
        element->preferredHeight = ButtonHeight;
        return button;
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

    auto headerGroup = Row(transform, ButtonHeight);
    auto headerRect = headerGroup->GetComponent<UnityEngine::RectTransform*>();
    headerRect->anchorMin = {0.5f, 1.0f};
    headerRect->anchorMax = {0.5f, 1.0f};
    headerRect->pivot = {0.5f, 1.0f};
    headerRect->sizeDelta = {SlotWidth, ButtonHeight};
    headerRect->anchoredPosition = {0.0f, HeaderLift};
    headerGroup->childForceExpandWidth = true;
    MakeButton(headerGroup->transform, "All", "PracticeButton", [] { Logger.info("All"); });
    MakeButton(headerGroup->transform, "None", "PracticeButton", [] { Logger.info("None"); });
    MakeButton(headerGroup->transform, "Invert", "PracticeButton", [] { Logger.info("Invert"); });

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

    auto list = BSML::Lite::CreateScrollableSettingsContainer(root->transform);
    auto listElement = Sized(list->transform, root->transform, ListHeight);
    auto listRoot = listElement->gameObject;
    auto catcherObject = UnityEngine::GameObject::New_ctor("ScrollCatcher");
    auto catcherRect = catcherObject->AddComponent<UnityEngine::RectTransform*>();
    UnityEngine::Transform* catcherParent = listRoot->transform;
    auto scroll = list->GetComponentInParent<HMUI::ScrollView*>();
    if (scroll) {
        catcherParent = scroll->transform;
    }
    Logger.info("Scroll view {} parent {}", scroll ? "found" : "missing", static_cast<std::string>(catcherParent->name));
    catcherRect->SetParent(catcherParent, false);
    catcherRect->anchorMin = {0.0f, 0.0f};
    catcherRect->anchorMax = {1.0f, 1.0f};
    catcherRect->offsetMin = {0.0f, 0.0f};
    catcherRect->offsetMax = {0.0f, 0.0f};
    catcherRect->SetAsFirstSibling();
    auto catcherLayout = catcherObject->AddComponent<UnityEngine::UI::LayoutElement*>();
    if (catcherLayout) {
        catcherLayout->ignoreLayout = true;
    }
    auto catcher = catcherObject->AddComponent<UnityEngine::UI::Image*>();
    if (catcher) {
        catcher->color = {0.0f, 0.0f, 0.0f, 0.01f};
        catcher->raycastTarget = true;
    } else {
        Logger.warn("Could not add the scroll catcher");
    }
    Logger.info("Catcher size {} x {}", catcherRect->rect.width, catcherRect->rect.height);
    for (auto name : SampleNames) {
        auto line = Row(list->transform, CoverSize + 1.0f);
        line->childAlignment = UnityEngine::TextAnchor::MiddleLeft;
        auto cover = UnityEngine::GameObject::New_ctor("Cover");
        cover->AddComponent<UnityEngine::RectTransform*>()->SetParent(line->transform, false);
        auto picture = cover->AddComponent<UnityEngine::UI::Image*>();
        picture->color = {0.25f, 0.55f, 0.95f, 1.0f};
        auto size = cover->AddComponent<UnityEngine::UI::LayoutElement*>();
        size->minWidth = CoverSize;
        size->preferredWidth = CoverSize;
        size->minHeight = CoverSize;
        size->preferredHeight = CoverSize;
        auto toggle = BSML::Lite::CreateToggle(line->transform, name, true, [name](bool value) {
            Logger.info("{} {}", name, value);
        });
        auto element = toggle->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!element) {
            element = toggle->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        }
        element->minWidth = PanelWidth - 22.0f;
        element->preferredWidth = PanelWidth - 22.0f;
        element->flexibleWidth = 0.0f;
    }

    auto footer = Row(root->transform, ButtonHeight);
    BSML::Lite::CreateText(footer->transform, "0 of 14 hidden", 3.5f, {0.0f, 0.0f}, {40.0f, 6.0f});
    MakeButton(footer->transform, "Cancel", "PracticeButton", [] { Logger.info("Cancel"); });
    MakeButton(footer->transform, "Apply", "PlayButton", [] { Logger.info("Apply"); });
}

namespace {
    template <typename T>
    void SetVisible(bool visible) {
        static UnityW<UnityEngine::CanvasGroup> group;
        try {
            if (!group) {
                auto all = UnityEngine::Resources::FindObjectsOfTypeAll<T*>();
                for (auto item : all) {
                    if (item && item->gameObject->activeInHierarchy) {
                        auto object = item->gameObject;
                        group = object->template GetComponent<UnityEngine::CanvasGroup*>();
                        if (!group) {
                            group = object->template AddComponent<UnityEngine::CanvasGroup*>();
                        }
                        break;
                    }
                }
            }
            if (!group) {
                return;
            }
            group->alpha = visible ? 1.0f : 0.0f;
            group->blocksRaycasts = visible;
            group->interactable = visible;
        } catch (...) {
            Logger.warn("Could not change the visibility");
        }
    }
}

namespace Panel {
    void SetListVisible(bool visible) {
        SetVisible<GlobalNamespace::LevelCollectionViewController>(visible);
        SetVisible<GlobalNamespace::LevelPackDetailViewController>(visible);
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
