#include "Panel.hpp"

#include <vector>

#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "UnityEngine/CanvasGroup.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/UI/Graphic.hpp"
#include "logger.hpp"

namespace {
    std::vector<UnityW<UnityEngine::UI::Graphic>> muted;
    bool listHidden = false;
    int hiddenTicks = 0;

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
            ReportError("could not mute raycasts", e);
        }
    }

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
            Logger.error("HIDEPACKS ERROR in changing the visibility: unknown error. Please send this file when reporting it.");
        }
    }

    void ApplyVisibility(bool visible) {
        SetVisible<GlobalNamespace::LevelCollectionViewController>(visible);
        SetVisible<GlobalNamespace::LevelPackDetailViewController>(visible);
        SetVisible<GlobalNamespace::StandardLevelDetailViewController>(visible);
        SetVisible<GlobalNamespace::LevelCollectionNavigationController>(visible);
    }
}

namespace Panel {
    void SetListVisible(bool visible) {
        listHidden = !visible;
        ApplyVisibility(visible);
        SetRaycasts(visible);
    }

    void KeepListHidden() {
        if (!listHidden) {
            return;
        }
        ApplyVisibility(false);
        if (++hiddenTicks % 30 == 0) {
            SetRaycasts(false);
        }
    }
}
