#include "Panel.hpp"

#include <functional>
#include <string>
#include <vector>

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/Touchable.hpp"
#include "PackState.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/Graphic.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/Helpers/creation.hpp"
#include "logger.hpp"

DEFINE_TYPE(HidePacks, PanelViewController);

namespace {
    constexpr float PanelWidth = 85.0f;
    constexpr float PanelHeight = 62.0f;
    constexpr float ListHeight = 40.0f;
    constexpr float SlotWidth = 95.0f;
    constexpr float SlotHeight = 10.0f;
    constexpr float CanvasHeight = 120.0f;
    constexpr float CanvasLift = (CanvasHeight - SlotHeight) / 2.0f;
    constexpr float PanelTop = 12.0f;
    constexpr float CoverSize = 6.0f;
    constexpr float ButtonHeight = 8.0f;
    constexpr float HeaderHeight = 9.6f;
    constexpr float HeaderGap = 5.0f;
    constexpr float HeaderButton = (PanelWidth - HeaderGap) / 2.0f;
    constexpr float FooterGap = 2.5f;
    constexpr float FooterButton = 18.0f;
    constexpr float FooterText = PanelWidth - 3.0f * FooterButton - 3.0f * FooterGap;

    struct RowState {
        UnityW<UnityEngine::Transform> transform;
        std::vector<UnityW<UnityEngine::UI::Graphic>> graphics;
        bool inside = true;
    };
    std::vector<RowState> rowStates;
    UnityW<UnityEngine::RectTransform> viewportRect;
    UnityW<UnityEngine::Transform> listContent;

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

    UnityEngine::UI::Button* MakeButton(UnityEngine::Transform* parent, char const* text, std::string_view style, float height, std::function<void()> onClick) {
        auto guarded = [text = std::string(text), onClick = std::move(onClick)] {
            try {
                onClick();
            } catch (std::exception const& e) {
                ReportError("button " + text, e);
            }
        };
        auto button = BSML::Lite::CreateUIButton(parent, text, style, {0.0f, 0.0f}, {0.0f, 0.0f}, std::move(guarded));
        if (!button) {
            return button;
        }
        auto element = button->gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!element) {
            element = button->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        }
        element->minHeight = height;
        element->preferredHeight = height;
        return button;
    }

    void Width(UnityEngine::Component* item, float width) {
        auto element = item->gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!element) {
            element = item->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        }
        element->minWidth = width;
        element->preferredWidth = width;
        element->flexibleWidth = 0.0f;
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

    void AddScrollCatcher(UnityEngine::Transform* parent) {
        auto object = UnityEngine::GameObject::New_ctor("ScrollCatcher");
        auto rect = object->AddComponent<UnityEngine::RectTransform*>();
        rect->SetParent(parent, false);
        rect->anchorMin = {0.0f, 0.0f};
        rect->anchorMax = {1.0f, 1.0f};
        rect->offsetMin = {0.0f, 0.0f};
        rect->offsetMax = {0.0f, 0.0f};
        rect->SetAsFirstSibling();
        auto layout = object->AddComponent<UnityEngine::UI::LayoutElement*>();
        if (layout) {
            layout->ignoreLayout = true;
        }
        auto image = object->AddComponent<UnityEngine::UI::Image*>();
        if (image) {
            image->color = {0.0f, 0.0f, 0.0f, 0.01f};
            image->raycastTarget = true;
        } else {
            Logger.warn("Could not add the scroll catcher");
        }
    }

    void AddPackRow(UnityEngine::Transform* list, PackState::Entry const& entry) {
        std::string id = entry.id;
        auto line = Row(list, CoverSize + 1.0f);
        rowStates.push_back({line->transform, {}, true});
        PackState::AddRow(line->transform);
        line->childAlignment = UnityEngine::TextAnchor::MiddleLeft;
        auto cover = entry.cover;
        auto picture = BSML::Lite::CreateImage(line->transform, cover.ptr(), {0.0f, 0.0f}, {CoverSize, CoverSize});
        picture->preserveAspect = true;
        auto size = picture->gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!size) {
            size = picture->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        }
        size->minWidth = CoverSize;
        size->preferredWidth = CoverSize;
        size->minHeight = CoverSize;
        size->preferredHeight = CoverSize;
        auto toggle = BSML::Lite::CreateToggle(line->transform, entry.name, PackState::IsVisible(id), [id](bool visible) {
            try {
                PackState::OnSwitch(id, visible);
            } catch (std::exception const& e) {
                ReportError("switch " + id, e);
            }
        });
        PackState::AddToggle(toggle);
        auto element = toggle->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!element) {
            element = toggle->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        }
        element->minWidth = PanelWidth - 22.0f;
        element->preferredWidth = PanelWidth - 22.0f;
        element->flexibleWidth = 0.0f;
    }

    void BuildRows(UnityEngine::Transform* list) {
        PackState::BeginSession();
        rowStates.clear();
        for (auto const& entry : PackState::Entries()) {
            AddPackRow(list, entry);
        }
        for (auto graphic : list->GetComponentsInChildren<UnityEngine::UI::Graphic*>(true)) {
            graphic->raycastTarget = true;
        }
        PackState::MarkBuilt();
    }

    void RebuildRows() {
        if (!listContent) {
            return;
        }
        for (auto& row : rowStates) {
            if (row.transform) {
                UnityEngine::Object::Destroy(row.transform->gameObject);
            }
        }
        BuildRows(listContent.ptr());
        PackState::Refresh();
        Logger.info("Rebuilt the pack rows");
    }
}

void HidePacks::PanelViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemDisabling) {
    if (!firstActivation) {
        return;
    }
    try {
        gameObject->AddComponent<HMUI::Touchable*>();

        auto headerGroup = Row(transform, HeaderHeight);
        auto headerRect = headerGroup->GetComponent<UnityEngine::RectTransform*>();
        headerRect->anchorMin = {0.5f, 1.0f};
        headerRect->anchorMax = {0.5f, 1.0f};
        headerRect->pivot = {0.5f, 1.0f};
        headerRect->sizeDelta = {PanelWidth, HeaderHeight};
        headerRect->anchoredPosition = {0.0f, -CanvasLift};
        headerGroup->childForceExpandWidth = false;
        headerGroup->spacing = HeaderGap;
        Width(MakeButton(headerGroup->transform, "All", "PracticeButton", HeaderHeight, [] { PackState::FilterRows(false); }), HeaderButton);
        Width(MakeButton(headerGroup->transform, "Only DLC", "PracticeButton", HeaderHeight, [] { PackState::FilterRows(true); }), HeaderButton);

        auto root = BSML::Lite::CreateVerticalLayoutGroup(transform);
        auto rootRect = root->GetComponent<UnityEngine::RectTransform*>();
        rootRect->anchorMin = {0.5f, 1.0f};
        rootRect->anchorMax = {0.5f, 1.0f};
        rootRect->pivot = {0.5f, 1.0f};
        rootRect->sizeDelta = {PanelWidth, PanelHeight};
        rootRect->anchoredPosition = {0.0f, -PanelTop - CanvasLift};
        root->spacing = 1.5f;
        root->childAlignment = UnityEngine::TextAnchor::UpperCenter;
        root->childControlWidth = true;
        root->childControlHeight = true;
        root->childForceExpandWidth = true;
        root->childForceExpandHeight = false;

        auto list = BSML::Lite::CreateScrollableSettingsContainer(root->transform);
        auto listElement = Sized(list->transform, root->transform, ListHeight);
        auto scroll = list->GetComponentInParent<HMUI::ScrollView*>();
        AddScrollCatcher(scroll ? scroll->transform : listElement->gameObject->transform);

        listContent = list->transform;
        BuildRows(list->transform);

        auto footer = Row(root->transform, ButtonHeight);
        footer->spacing = FooterGap;
        auto counter = BSML::Lite::CreateText(footer->transform, "", 3.5f, {0.0f, 0.0f}, {FooterText, 6.0f});
        PackState::BindCounter(counter);
        Width(counter, FooterText);
        Width(MakeButton(footer->transform, "Reset", "PracticeButton", ButtonHeight, [] { PackState::ShowAll(); }), FooterButton);
        Width(MakeButton(footer->transform, "Cancel", "PracticeButton", ButtonHeight, [] { PackState::Reset(); }), FooterButton);
        auto apply = MakeButton(footer->transform, "Apply", "PlayButton", ButtonHeight, [] { PackState::Apply(); });
        PackState::BindApply(apply);
        Width(apply, FooterButton);
        PackState::Refresh();

        if (scroll) {
            auto viewportTransform = scroll->transform->Find("Viewport");
            if (viewportTransform) {
                viewportRect = viewportTransform->GetComponent<UnityEngine::RectTransform*>();
            }
        }
    } catch (std::exception const& e) {
        ReportError("could not build the panel", e);
    }
}

namespace Panel {
    void Show(GlobalNamespace::LevelFilteringNavigationController* navigation) {
        static UnityW<HidePacks::PanelViewController> panel;
        PackState::Load(navigation);
        if (!panel) {
            panel = BSML::Helpers::CreateViewController<HidePacks::PanelViewController*>();
            auto rect = panel->GetComponent<UnityEngine::RectTransform*>();
            rect->anchorMin = {0.5f, 0.5f};
            rect->anchorMax = {0.5f, 0.5f};
            rect->pivot = {0.5f, 0.5f};
            rect->sizeDelta = {SlotWidth, CanvasHeight};
        }
        navigation->ReplaceSecondViewController(panel.ptr());
        if (PackState::RowsStale()) {
            RebuildRows();
        }
        SetListVisible(false);
        PackState::Reset();
        PackState::FilterRows(false);
    }
}

void HidePacks::PanelViewController::Update() {
    try {
        PackState::Tick();
        Panel::KeepListHidden();
        if (!viewportRect) {
            return;
        }
        auto bounds = viewportRect->rect;
        for (auto& row : rowStates) {
            if (!row.transform) {
                continue;
            }
            auto center = viewportRect->InverseTransformPoint(row.transform->position);
            bool inside = center.y >= bounds.yMin && center.y <= bounds.yMax;
            if (inside == row.inside && !row.graphics.empty()) {
                continue;
            }
            row.inside = inside;
            if (row.graphics.empty()) {
                for (auto graphic : row.transform->GetComponentsInChildren<UnityEngine::UI::Graphic*>(true)) {
                    row.graphics.push_back(graphic);
                }
            }
            for (auto graphic : row.graphics) {
                if (graphic) {
                    graphic->raycastTarget = inside;
                }
            }
        }
    } catch (std::exception const& e) {
        ReportError("panel update failed", e);
    }
}
