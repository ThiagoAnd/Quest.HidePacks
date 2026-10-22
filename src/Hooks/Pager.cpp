#include "autohooks/shared/hooks.hpp"
#include "logger.hpp"

#include <algorithm>
#include <cmath>

#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/IconSegmentedControlCell.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/LayoutRebuilder.hpp"
#include "UnityEngine/UI/RectMask2D.hpp"
#include "UnityEngine/Vector3.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/BSML/Components/ClickableText.hpp"

using namespace GlobalNamespace;

namespace {
    constexpr const char* ViewportName = "HidePacksViewport";

    UnityW<UnityEngine::RectTransform> viewport;
    UnityW<UnityEngine::RectTransform> bar;
    UnityW<UnityEngine::RectTransform> control;
    UnityW<UnityEngine::GameObject> leftArrow;
    UnityW<UnityEngine::GameObject> rightArrow;
    UnityW<HMUI::CurvedTextMeshPro> indicator;
    float pitch = 0.0f;
    float originX = 0.0f;
    float top = 0.0f;
    float height = 0.0f;
    int slots = 0;
    int count = 0;
    int page = 0;

    UnityEngine::Vector3 BarPoint(UnityEngine::RectTransform* rect, int corner) {
        ArrayW<UnityEngine::Vector3> corners(4);
        rect->GetWorldCorners(corners);
        return bar->InverseTransformPoint(corners[corner]);
    }

    UnityEngine::RectTransform* MakeArrow(char const* text, std::function<void()> onClick) {
        auto label = BSML::Lite::CreateClickableText(bar.ptr(), text, 3.5f, {0, 0}, {pitch, height}, std::move(onClick));
        label->set_alignment(TMPro::TextAlignmentOptions::Center);
        auto rect = label->GetComponent<UnityEngine::RectTransform*>();
        rect->anchorMin = {0.0f, 1.0f};
        rect->anchorMax = {0.0f, 1.0f};
        rect->pivot = {0.0f, 1.0f};
        rect->sizeDelta = {pitch, height};
        return rect;
    }

    int PageCount() {
        return (count + slots - 1) / slots;
    }

    void Apply() {
        auto iconControl = control->GetComponent<HMUI::IconSegmentedControl*>();
        UnityEngine::UI::LayoutRebuilder::ForceRebuildLayoutImmediate(control.ptr());
        auto first = iconControl->_cells->get_Item(0)->GetComponent<UnityEngine::RectTransform*>();
        ArrayW<UnityEngine::Vector3> corners(4);
        first->GetWorldCorners(corners);
        float current = viewport->InverseTransformPoint(corners[1]).x;
        float target = -page * slots * pitch;
        auto shift = viewport->TransformVector({target - current, 0.0f, 0.0f});
        auto position = control->position;
        control->position = {position.x + shift.x, position.y + shift.y, position.z + shift.z};

        indicator->set_text(std::to_string(page + 1) + "/" + std::to_string(PageCount()));
        leftArrow->SetActive(page > 0);
        rightArrow->SetActive(page < PageCount() - 1);
        Logger.info("Tabs {} page {} of {} first cell was {} slots from target", count, page + 1, PageCount(), (current - target) / pitch);
    }

    void Next() {
        page = std::min(page + 1, PageCount() - 1);
        Apply();
    }

    void Previous() {
        page = std::max(page - 1, 0);
        Apply();
    }

    void Build(SelectLevelCategoryViewController* self) {
        auto iconControl = self->_levelFilterCategoryIconSegmentedControl;
        if (!iconControl || !iconControl->_cells) {
            return;
        }
        count = iconControl->_numberOfCells;
        auto controlRect = iconControl->GetComponent<UnityEngine::RectTransform*>();
        auto barRect = self->GetComponent<UnityEngine::RectTransform*>();
        if (!controlRect || !barRect || count < 2) {
            return;
        }
        bar = barRect;
        UnityEngine::UI::LayoutRebuilder::ForceRebuildLayoutImmediate(controlRect);
        auto first = iconControl->_cells->get_Item(0)->GetComponent<UnityEngine::RectTransform*>();
        auto second = iconControl->_cells->get_Item(1)->GetComponent<UnityEngine::RectTransform*>();
        auto firstLeft = BarPoint(first, 1);
        auto secondLeft = BarPoint(second, 1);
        auto firstBottom = BarPoint(first, 0);
        pitch = secondLeft.x - firstLeft.x;
        if (pitch <= 0.0f) {
            pitch = BarPoint(first, 2).x - firstLeft.x;
        }
        originX = firstLeft.x;
        top = firstLeft.y;
        height = firstLeft.y - firstBottom.y;
        if (pitch <= 0.0f || height <= 0.0f) {
            Logger.warn("Unexpected cell geometry pitch {} height {}", pitch, height);
            return;
        }
        slots = static_cast<int>(std::round(barRect->rect.width / pitch));
        if (count <= slots) {
            Logger.info("{} tabs fit in {} slots, no pager", count, slots);
            return;
        }

        if (!viewport || controlRect->parent != viewport.ptr()) {
            auto go = UnityEngine::GameObject::New_ctor(ViewportName);
            auto rect = go->AddComponent<UnityEngine::RectTransform*>();
            go->AddComponent<UnityEngine::UI::RectMask2D*>();
            rect->SetParent(controlRect->parent, false);
            rect->anchorMin = {0.0f, 1.0f};
            rect->anchorMax = {0.0f, 1.0f};
            rect->pivot = {0.0f, 1.0f};
            rect->position = bar->TransformPoint({originX, top, 0.0f});
            auto size = rect->parent.cast<UnityEngine::RectTransform>()->InverseTransformVector(
                bar->TransformVector({slots * pitch, height, 0.0f})
            );
            rect->sizeDelta = {std::abs(size.x), std::abs(size.y)};
            viewport = rect;
            auto position = controlRect->position;
            auto controlSize = controlRect->sizeDelta;
            controlRect->SetParent(rect, true);
            controlRect->position = position;
            controlRect->sizeDelta = controlSize;
            Logger.info("Viewport created, pitch {} height {} slots {} tabs {}", pitch, height, slots, count);
        }
        control = controlRect;

        float arrowHeight = height * 0.6f;
        auto arrowParent = viewport->parent;
        auto place = [&](UnityEngine::RectTransform* item, float column, float width) {
            item->SetParent(arrowParent, false);
            item->anchorMin = {0.0f, 1.0f};
            item->anchorMax = {0.0f, 1.0f};
            item->pivot = {0.0f, 1.0f};
            item->sizeDelta = {width * pitch, arrowHeight};
            auto at = viewport->anchoredPosition;
            item->anchoredPosition = {at.x + column * pitch, at.y - height};
        };
        float buttonWidth = 1.5f;
        if (!leftArrow) {
            auto left = MakeArrow("Back", [] { Previous(); });
            place(left, 0.0f, buttonWidth);
            leftArrow = left->gameObject;
        }
        if (!rightArrow) {
            auto right = MakeArrow("Next", [] { Next(); });
            place(right, slots - buttonWidth, buttonWidth);
            rightArrow = right->gameObject;
        }
        if (!indicator) {
            indicator = BSML::Lite::CreateText(bar.ptr(), "", 3.5f, {0, 0}, {pitch, height});
            indicator->set_alignment(TMPro::TextAlignmentOptions::Center);
            place(indicator->GetComponent<UnityEngine::RectTransform*>(), buttonWidth, slots - 2 * buttonWidth);
        }
        page = std::min(page, PageCount() - 1);
        Apply();
    }
}

MAKE_LATE_HOOK_MATCH(
    SelectLevelCategoryViewController_DidActivate,
    &SelectLevelCategoryViewController::DidActivate,
    void,
    SelectLevelCategoryViewController* self,
    bool firstActivation,
    bool addedToHierarchy,
    bool screenSystemEnabling
) {
    SelectLevelCategoryViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    try {
        Build(self);
    } catch (std::exception const& e) {
        ReportError("pager failed", e);
    }
}
