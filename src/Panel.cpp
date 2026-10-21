#include "Panel.hpp"

#include <array>
#include <cmath>
#include <string>
#include <vector>

#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/Touchable.hpp"
#include "UnityEngine/CanvasGroup.hpp"
#include "UnityEngine/Bounds.hpp"
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
    constexpr float HeaderHeight = 9.6f;
    constexpr float HeaderLift = 0.0f;


    struct RowState {
        UnityW<UnityEngine::Transform> transform;
        std::vector<UnityW<UnityEngine::UI::Graphic>> graphics;
        bool inside = true;
    };
    std::vector<RowState> rowStates;
    UnityW<UnityEngine::RectTransform> viewportRect;
    UnityW<UnityEngine::RectTransform> probeFooter;
    UnityW<UnityEngine::RectTransform> probeList;

    std::string PathOf(UnityEngine::Transform* transform) {
        std::string path;
        int depth = 0;
        while (transform && depth < 12) {
            path = static_cast<std::string>(transform->name) + "/" + path;
            transform = transform->parent;
            depth++;
        }
        return path;
    }

    bool Near(UnityEngine::Bounds bounds, UnityEngine::Vector3 point) {
        auto center = bounds.get_center();
        auto extents = bounds.get_extents();
        return std::abs(point.x - center.x) <= extents.x + 1.0f && std::abs(point.y - center.y) <= extents.y + 1.0f && std::abs(point.z - center.z) <= extents.z + 1.0f;
    }

    void Probe() {
        try {
            if (probeList) {
                Logger.info("List rect {} x {} at {} {} children {}", probeList->rect.width, probeList->rect.height, probeList->anchoredPosition.x, probeList->anchoredPosition.y, probeList->childCount);
            }
            if (!probeFooter) {
                return;
            }
            auto all = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::UI::Graphic*>();
            for (auto button : probeFooter->GetComponentsInChildren<UnityEngine::UI::Button*>(true)) {
                auto point = button->transform->position;
                Logger.info("Under button {}", static_cast<std::string>(button->gameObject->name));
                for (auto collider : UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Collider*>()) {
                    if (collider && collider->enabled && collider->gameObject->activeInHierarchy && Near(collider->bounds, point)) {
                        Logger.info("  collider {}", PathOf(collider->transform));
                    }
                }
                for (auto graphic : all) {
                    if (!graphic || !graphic->raycastTarget || !graphic->gameObject->activeInHierarchy || !graphic->enabled) {
                        continue;
                    }
                    auto local = graphic->rectTransform->InverseTransformPoint(point);
                    auto rect = graphic->rectTransform->rect;
                    if (local.x >= rect.xMin && local.x <= rect.xMax && local.y >= rect.yMin && local.y <= rect.yMax && std::abs(local.z) < 5.0f) {
                        Logger.info("  z {} {}", local.z, PathOf(graphic->transform));
                    }
                }
            }
        } catch (std::exception const& e) {
            Logger.warn("Probe failed: {}", e.what());
        }
    }

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

void HidePacks::PanelViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemDisabling) {
    if (!firstActivation) {
        return;
    }
    gameObject->AddComponent<HMUI::Touchable*>();

    auto headerGroup = Row(transform, HeaderHeight);
    auto headerRect = headerGroup->GetComponent<UnityEngine::RectTransform*>();
    headerRect->anchorMin = {0.5f, 1.0f};
    headerRect->anchorMax = {0.5f, 1.0f};
    headerRect->pivot = {0.5f, 1.0f};
    headerRect->sizeDelta = {SlotWidth, HeaderHeight};
    headerRect->anchoredPosition = {0.0f, HeaderLift};
    headerGroup->childForceExpandWidth = true;
    MakeButton(headerGroup->transform, "All", "PracticeButton", HeaderHeight, [] { Logger.info("All"); Probe(); });
    MakeButton(headerGroup->transform, "None", "PracticeButton", HeaderHeight, [] { Logger.info("None"); });
    MakeButton(headerGroup->transform, "Invert", "PracticeButton", HeaderHeight, [] { Logger.info("Invert"); });

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
        rowStates.push_back({line->transform, {}, true});
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

    for (auto graphic : list->GetComponentsInChildren<UnityEngine::UI::Graphic*>(true)) {
        graphic->raycastTarget = true;
    }

    auto footer = Row(root->transform, ButtonHeight);
    auto footerRect = footer->GetComponent<UnityEngine::RectTransform*>();
    BSML::Lite::CreateText(footer->transform, "0 of 14 hidden", 3.5f, {0.0f, 0.0f}, {40.0f, 6.0f});
    MakeButton(footer->transform, "Cancel", "PracticeButton", ButtonHeight, [] { Logger.info("Cancel"); });
    MakeButton(footer->transform, "Apply", "PlayButton", ButtonHeight, [] { Logger.info("Apply"); });
    if (scroll) {
        auto viewportTransform = scroll->transform->Find("Viewport");
        if (viewportTransform) {
            viewportRect = viewportTransform->GetComponent<UnityEngine::RectTransform*>();
        }
    }
    probeFooter = footerRect;
    probeList = list->GetComponent<UnityEngine::RectTransform*>();
}

namespace {
    std::vector<UnityW<UnityEngine::UI::Graphic>> muted;

    void SetRaycasts(bool enabled) {
        try {
            if (enabled) {
                for (auto graphic : muted) {
                    if (graphic) {
                        graphic->raycastTarget = true;
                    }
                }
                muted.clear();
                return;
            }
            for (auto navigation : UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionNavigationController*>()) {
                if (!navigation || !navigation->gameObject->activeInHierarchy) {
                    continue;
                }
                for (auto graphic : navigation->GetComponentsInChildren<UnityEngine::UI::Graphic*>(true)) {
                    if (graphic && graphic->raycastTarget) {
                        graphic->raycastTarget = false;
                        muted.push_back(graphic);
                    }
                }
                break;
            }
        } catch (std::exception const& e) {
            Logger.warn("Could not mute raycasts: {}", e.what());
        }
    }
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
        SetVisible<GlobalNamespace::LevelCollectionNavigationController>(visible);
        SetRaycasts(visible);
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

void HidePacks::PanelViewController::Update() {
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
