#include "Panel.hpp"

#include <array>
#include <cmath>
#include <string>
#include <vector>

#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/Touchable.hpp"
#include "UnityEngine/CanvasGroup.hpp"
#include "UnityEngine/Bounds.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "UnityEngine/Color.hpp"
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
DEFINE_TYPE(HidePacks, PaneViewController);

namespace {
    constexpr float SlotWidth = 95.0f;
    constexpr float SlotHeight = 10.0f;
    constexpr float CanvasHeight = 120.0f;
    constexpr float PanelTop = 12.0f;
    constexpr float PaneHeight = 56.0f;
    constexpr float PaneCenterY = -(PanelTop - SlotHeight / 2.0f) - PaneHeight / 2.0f;
    constexpr float ListWidth = 75.0f;
    constexpr float ListX = -54.0f;
    constexpr float SideWidth = 70.0f;
    constexpr float SideX = 22.5f;
    constexpr float ToggleWidth = 53.0f;
    constexpr float CoverSize = 6.0f;
    constexpr float ButtonHeight = 8.0f;

    struct RowState {
        UnityW<UnityEngine::Transform> transform;
        std::vector<UnityW<UnityEngine::UI::Graphic>> graphics;
        bool inside = true;
    };
    std::vector<RowState> rowStates;
    UnityW<UnityEngine::RectTransform> viewportRect;
    bool listHidden = false;
    int hiddenTicks = 0;

    constexpr std::array<char const*, 14> SampleNames = {
        "Original Soundtrack Vol. 1", "Original Soundtrack Vol. 2", "Original Soundtrack Vol. 3",
        "Original Soundtrack Vol. 4", "Original Soundtrack Vol. 5", "Original Soundtrack Vol. 6",
        "Original Soundtrack Vol. 7", "Extras", "Camellia Music Pack", "Monstercat Music Pack Vol. 1",
        "Rocket League x Monstercat", "Green Day Music Pack", "Timbaland Music Pack", "BTS Music Pack"
    };

    constexpr std::array<char const*, 14> SampleKinds = {
        "OST", "OST", "OST", "OST", "OST", "OST", "OST", "OST", "DLC", "DLC", "DLC", "DLC", "DLC", "DLC"
    };
    constexpr std::array<int, 14> SampleSongs = {11, 10, 10, 10, 10, 10, 10, 9, 8, 7, 6, 8, 7, 7};

    std::array<bool, 14> visibleState = {true, true, true, true, true, true, true, true, true, true, true, true, true, true};
    int selected = 0;
    UnityW<TMPro::TextMeshProUGUI> detailName;
    UnityW<TMPro::TextMeshProUGUI> detailInfo;
    UnityW<TMPro::TextMeshProUGUI> summary;

    void Refresh() {
        int hidden = 0;
        for (bool value : visibleState) {
            if (!value) {
                hidden++;
            }
        }
        auto total = static_cast<int>(visibleState.size());
        if (summary) {
            summary->set_text(fmt::format("{} hidden | {} visible | {} total", hidden, total - hidden, total));
        }
        if (detailName) {
            detailName->set_text(SampleNames[selected]);
        }
        if (detailInfo) {
            detailInfo->set_text(fmt::format("{} | {} songs | {}", SampleKinds[selected], SampleSongs[selected], visibleState[selected] ? "Visible" : "Hidden"));
        }
    }

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
        auto button = BSML::Lite::CreateUIButton(parent, text, style, {0.0f, 0.0f}, {0.0f, 0.0f}, std::move(onClick));
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

namespace {
    UnityEngine::Transform* MakePane(UnityEngine::Transform* parent, float x, float width) {
        auto pane = BSML::Helpers::CreateViewController<HidePacks::PaneViewController*>();
        auto rect = pane->GetComponent<UnityEngine::RectTransform*>();
        rect->SetParent(parent, false);
        rect->anchorMin = {0.5f, 0.5f};
        rect->anchorMax = {0.5f, 0.5f};
        rect->pivot = {0.5f, 0.5f};
        rect->sizeDelta = {width, PaneHeight};
        rect->anchoredPosition = {x, PaneCenterY};
        pane->gameObject->AddComponent<HMUI::Touchable*>();
        auto group = pane->gameObject->GetComponent<UnityEngine::CanvasGroup*>();
        if (group) {
            group->alpha = 1.0f;
            group->blocksRaycasts = true;
            group->interactable = true;
        }
        pane->gameObject->SetActive(true);
        return pane->transform;
    }

    UnityEngine::UI::VerticalLayoutGroup* Fill(UnityEngine::Transform* pane, float spacing) {
        auto root = BSML::Lite::CreateVerticalLayoutGroup(pane);
        auto rect = root->GetComponent<UnityEngine::RectTransform*>();
        rect->anchorMin = {0.0f, 0.0f};
        rect->anchorMax = {1.0f, 1.0f};
        rect->offsetMin = {0.0f, 0.0f};
        rect->offsetMax = {0.0f, 0.0f};
        root->spacing = spacing;
        root->childAlignment = UnityEngine::TextAnchor::UpperCenter;
        root->childControlWidth = true;
        root->childControlHeight = true;
        root->childForceExpandWidth = true;
        root->childForceExpandHeight = false;
        return root;
    }

    UnityEngine::UI::HorizontalLayoutGroup* ButtonRow(UnityEngine::Transform* root, float height) {
        auto row = Row(root, height);
        row->childForceExpandWidth = true;
        return row;
    }

    void BuildSide(UnityEngine::Transform* pane) {
        constexpr float Compact = 7.0f;
        constexpr float CoverBig = 14.0f;
        auto root = Fill(pane, 1.5f);
        auto title = BSML::Lite::CreateText(root->transform, "Hide Packs", 5.0f, {0.0f, 0.0f}, {SideWidth, 7.0f});
        title->set_alignment(TMPro::TextAlignmentOptions::Center);
        Sized(title->transform, root->transform, 7.0f);

        auto detail = Row(root->transform, CoverBig);
        detail->spacing = 3.0f;
        detail->childAlignment = UnityEngine::TextAnchor::MiddleLeft;
        auto cover = UnityEngine::GameObject::New_ctor("DetailCover");
        cover->AddComponent<UnityEngine::RectTransform*>()->SetParent(detail->transform, false);
        auto picture = cover->AddComponent<UnityEngine::UI::Image*>();
        picture->color = {0.25f, 0.55f, 0.95f, 1.0f};
        auto coverSize = cover->AddComponent<UnityEngine::UI::LayoutElement*>();
        coverSize->minWidth = CoverBig;
        coverSize->preferredWidth = CoverBig;
        coverSize->minHeight = CoverBig;
        coverSize->preferredHeight = CoverBig;
        auto texts = BSML::Lite::CreateVerticalLayoutGroup(detail->transform);
        texts->spacing = 0.5f;
        texts->childAlignment = UnityEngine::TextAnchor::MiddleLeft;
        texts->childControlWidth = true;
        texts->childControlHeight = true;
        texts->childForceExpandWidth = true;
        texts->childForceExpandHeight = false;
        auto textsSize = texts->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        textsSize->minWidth = SideWidth - CoverBig - 6.0f;
        textsSize->preferredWidth = SideWidth - CoverBig - 6.0f;
        detailName = BSML::Lite::CreateText(texts->transform, "", 3.8f, {0.0f, 0.0f}, {SideWidth - CoverBig - 6.0f, 6.0f});
        detailName->set_alignment(TMPro::TextAlignmentOptions::Left);
        detailName->set_enableWordWrapping(false);
        detailName->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
        Sized(detailName->transform, texts->transform, 6.0f);
        detailInfo = BSML::Lite::CreateText(texts->transform, "", 3.0f, {0.0f, 0.0f}, {SideWidth - CoverBig - 6.0f, 5.0f});
        detailInfo->set_alignment(TMPro::TextAlignmentOptions::Left);
        detailInfo->set_enableWordWrapping(false);
        Sized(detailInfo->transform, texts->transform, 5.0f);

        auto kinds = ButtonRow(root->transform, Compact);
        MakeButton(kinds->transform, "Only OST", "PracticeButton", Compact, [] { Logger.info("Only OST"); });
        MakeButton(kinds->transform, "Only DLC", "PracticeButton", Compact, [] { Logger.info("Only DLC"); });
        MakeButton(kinds->transform, "Only Custom", "PracticeButton", Compact, [] { Logger.info("Only Custom"); });

        auto selection = ButtonRow(root->transform, Compact);
        MakeButton(selection->transform, "All", "PracticeButton", Compact, [] { Logger.info("All"); });
        MakeButton(selection->transform, "None", "PracticeButton", Compact, [] { Logger.info("None"); });
        MakeButton(selection->transform, "Invert", "PracticeButton", Compact, [] { Logger.info("Invert"); });

        summary = BSML::Lite::CreateText(root->transform, "", 3.2f, {0.0f, 0.0f}, {SideWidth, 5.0f});
        summary->set_alignment(TMPro::TextAlignmentOptions::Center);
        Sized(summary->transform, root->transform, 5.0f);

        auto spacer = UnityEngine::GameObject::New_ctor("Spacer");
        spacer->AddComponent<UnityEngine::RectTransform*>()->SetParent(root->transform, false);
        auto spacerLayout = spacer->AddComponent<UnityEngine::UI::LayoutElement*>();
        spacerLayout->flexibleHeight = 1.0f;

        auto footer = ButtonRow(root->transform, ButtonHeight);
        MakeButton(footer->transform, "Cancel", "PracticeButton", ButtonHeight, [] { Logger.info("Cancel"); });
        MakeButton(footer->transform, "Apply", "PlayButton", ButtonHeight, [] { Logger.info("Apply"); });
        Refresh();
    }
}

void HidePacks::PanelViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemDisabling) {
    if (!firstActivation) {
        return;
    }
    gameObject->AddComponent<HMUI::Touchable*>();
    BuildSide(MakePane(transform, SideX, SideWidth));
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
        listHidden = !visible;
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
            rect->sizeDelta = {SlotWidth, CanvasHeight};
        }
        navigation->ReplaceSecondViewController(panel.ptr());
        SetListVisible(false);
    }
}

void HidePacks::PanelViewController::Update() {
    if (listHidden) {
        SetVisible<GlobalNamespace::LevelPackDetailViewController>(false);
    }
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
}
