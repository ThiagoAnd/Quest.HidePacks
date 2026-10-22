#include "Panel.hpp"

#include <array>
#include <cmath>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "Config.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/Touchable.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Sprite.hpp"
#include "bsml/shared/BSML/Components/Settings/ToggleSetting.hpp"
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

namespace {
    constexpr float PanelWidth = 85.0f;
    constexpr float PanelHeight = 62.0f;
    constexpr float ListHeight = 40.0f;
    constexpr float SlotWidth = 95.0f;
    constexpr float SlotHeight = 10.0f;
    constexpr float CanvasHeight = 120.0f;
    constexpr float CanvasLift = (CanvasHeight - SlotHeight) / 2.0f;
    constexpr float SlotCenter = 17.0f;
    constexpr float PanelTop = 12.0f;
    constexpr float CoverSize = 6.0f;
    constexpr float ButtonHeight = 8.0f;
    constexpr float HeaderHeight = 9.6f;
    constexpr float HeaderLift = 0.0f;
    constexpr float HeaderGap = 5.0f;
    constexpr float HeaderButton = (PanelWidth - 2.0f * HeaderGap) / 3.0f;
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
    bool listHidden = false;
    int hiddenTicks = 0;
    struct Entry {
        std::string id;
        std::string name;
        bool dlc = true;
        UnityW<UnityEngine::Sprite> cover;
    };
    std::vector<Entry> entries;
    std::unordered_set<std::string> pending;
    std::vector<UnityW<BSML::ToggleSetting>> toggles;
    UnityW<TMPro::TextMeshProUGUI> counter;
    bool syncing = false;
    bool blocked = false;

    bool IsBase(std::string const& id) {
        return id.rfind("OstVol", 0) == 0 || id == "Extras" || id == "Camellia";
    }

    int HiddenCount() {
        int count = 0;
        for (auto const& entry : entries) {
            if (pending.contains(entry.id)) {
                count++;
            }
        }
        return count;
    }

    void UpdateCounter() {
        if (!counter) {
            return;
        }
        int hidden = HiddenCount();
        int total = static_cast<int>(entries.size());
        if (blocked && hidden >= total) {
            counter->text = "Keep 1 visible";
            return;
        }
        counter->text = std::to_string(hidden) + "/" + std::to_string(total) + " hidden";
    }

    void SyncToggles() {
        syncing = true;
        for (size_t i = 0; i < toggles.size() && i < entries.size(); i++) {
            if (toggles[i]) {
                bool visible = !pending.contains(entries[i].id);
                if (toggles[i]->toggle->isOn != visible) {
                    toggles[i]->set_Value(visible);
                }
            }
        }
        syncing = false;
        UpdateCounter();
    }

    void Reset() {
        pending = Config::Hidden();
        blocked = false;
        SyncToggles();
    }

    void ShowRows(bool onlyDlc) {
        for (size_t i = 0; i < rowStates.size() && i < entries.size(); i++) {
            if (rowStates[i].transform) {
                rowStates[i].transform->gameObject->SetActive(!onlyDlc || entries[i].dlc);
            }
        }
    }

    void ShowAll() {
        pending.clear();
        blocked = false;
        SyncToggles();
    }

    void Apply() {
        if (!entries.empty() && HiddenCount() >= static_cast<int>(entries.size())) {
            blocked = true;
            UpdateCounter();
            return;
        }
        blocked = false;
        Config::SetHidden(pending);
        Config::Save();
        UpdateCounter();
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
    headerRect->sizeDelta = {PanelWidth, HeaderHeight};
    headerRect->anchoredPosition = {0.0f, HeaderLift - CanvasLift};
    headerGroup->childForceExpandWidth = false;
    headerGroup->spacing = HeaderGap;
    Width(MakeButton(headerGroup->transform, "All", "PracticeButton", HeaderHeight, [] { ShowRows(false); }), HeaderButton);
    Width(MakeButton(headerGroup->transform, "Only DLC", "PracticeButton", HeaderHeight, [] { ShowRows(true); }), HeaderButton);
    Width(MakeButton(headerGroup->transform, "Only Custom", "PracticeButton", HeaderHeight, [] { Logger.info("Only Custom"); }), HeaderButton);

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
    toggles.clear();
    rowStates.clear();
    pending = Config::Hidden();
    for (size_t i = 0; i < entries.size(); i++) {
        std::string id = entries[i].id;
        auto line = Row(list->transform, CoverSize + 1.0f);
        rowStates.push_back({line->transform, {}, true});
        line->childAlignment = UnityEngine::TextAnchor::MiddleLeft;
        auto cover = UnityEngine::GameObject::New_ctor("Cover");
        cover->AddComponent<UnityEngine::RectTransform*>()->SetParent(line->transform, false);
        auto picture = cover->AddComponent<UnityEngine::UI::Image*>();
        if (entries[i].cover) {
            picture->sprite = entries[i].cover.ptr();
            picture->preserveAspect = true;
        } else {
            picture->color = {0.25f, 0.55f, 0.95f, 1.0f};
        }
        auto size = cover->AddComponent<UnityEngine::UI::LayoutElement*>();
        size->minWidth = CoverSize;
        size->preferredWidth = CoverSize;
        size->minHeight = CoverSize;
        size->preferredHeight = CoverSize;
        auto toggle = BSML::Lite::CreateToggle(line->transform, entries[i].name, !pending.contains(id), [id](bool value) {
            if (syncing) {
                return;
            }
            if (value) {
                pending.erase(id);
            } else {
                pending.insert(id);
            }
            blocked = false;
            UpdateCounter();
        });
        toggles.push_back(toggle);
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
    footer->spacing = FooterGap;
    counter = BSML::Lite::CreateText(footer->transform, "", 3.5f, {0.0f, 0.0f}, {FooterText, 6.0f});
    Width(counter, FooterText);
    Width(MakeButton(footer->transform, "Reset", "PracticeButton", ButtonHeight, [] { ShowAll(); }), FooterButton);
    Width(MakeButton(footer->transform, "Cancel", "PracticeButton", ButtonHeight, [] { Reset(); }), FooterButton);
    Width(MakeButton(footer->transform, "Apply", "PlayButton", ButtonHeight, [] { Apply(); }), FooterButton);
    UpdateCounter();
    if (scroll) {
        auto viewportTransform = scroll->transform->Find("Viewport");
        if (viewportTransform) {
            viewportRect = viewportTransform->GetComponent<UnityEngine::RectTransform*>();
        }
    }
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
        listHidden = !visible;
        SetVisible<GlobalNamespace::LevelCollectionViewController>(visible);
        SetVisible<GlobalNamespace::LevelPackDetailViewController>(visible);
        SetVisible<GlobalNamespace::StandardLevelDetailViewController>(visible);
        SetVisible<GlobalNamespace::LevelCollectionNavigationController>(visible);
        SetRaycasts(visible);
    }

    void LoadEntries(GlobalNamespace::LevelFilteringNavigationController* navigation) {
        std::vector<Entry> found;
        int missing = 0;
        try {
            auto packs = navigation->_allOfficialBeatmapLevelPacks;
            if (packs) {
                for (auto pack : packs) {
                    if (!pack) {
                        continue;
                    }
                    Entry entry;
                    entry.id = static_cast<std::string>(pack->packID);
                    entry.name = static_cast<std::string>(pack->packName);
                    entry.dlc = !IsBase(entry.id);
                    entry.cover = pack->coverImage;
            if (!entry.cover) {
                entry.cover = pack->smallCoverImage;
            }
            if (!entry.cover) {
                missing++;
            }
                    found.push_back(std::move(entry));
                }
            }
        } catch (std::exception const& e) {
            Logger.warn("Could not read the packs: {}", e.what());
        }
        if (!found.empty()) {
            entries = std::move(found);
        }
        Logger.info("Panel has {} packs, {} without cover", entries.size(), missing);
    }

    void Show(GlobalNamespace::LevelFilteringNavigationController* navigation) {
        static UnityW<HidePacks::PanelViewController> panel;
        LoadEntries(navigation);
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
        Reset();
        ShowRows(false);
    }
}

void HidePacks::PanelViewController::Update() {
    if (listHidden) {
        SetVisible<GlobalNamespace::LevelCollectionViewController>(false);
        SetVisible<GlobalNamespace::LevelPackDetailViewController>(false);
        SetVisible<GlobalNamespace::StandardLevelDetailViewController>(false);
        SetVisible<GlobalNamespace::LevelCollectionNavigationController>(false);
        if (++hiddenTicks % 30 == 0) {
            SetRaycasts(false);
        }
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
