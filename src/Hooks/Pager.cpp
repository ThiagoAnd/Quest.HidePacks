#include "autohooks/shared/hooks.hpp"
#include "logger.hpp"

#include <algorithm>
#include <cmath>

#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/IconSegmentedControlCell.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/RectTransform.hpp"
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
    UnityEngine::Vector3 baseWorld;
    float pitch = 0.0f;
    float originX = 0.0f;
    float top = 0.0f;
    float height = 0.0f;
    int slots = 0;
    int count = 0;
    int offset = 0;

    UnityEngine::Vector3 BarPoint(UnityEngine::RectTransform* rect, int corner) {
        ArrayW<UnityEngine::Vector3> corners(4);
        rect->GetWorldCorners(corners);
        return bar->InverseTransformPoint(corners[corner]);
    }

    UnityEngine::RectTransform* MakeArrow(char const* text, std::function<void()> onClick) {
        auto label = BSML::Lite::CreateClickableText(bar.ptr(), text, 6.0f, {0, 0}, {pitch, height}, std::move(onClick));
        label->set_alignment(TMPro::TextAlignmentOptions::Center);
        auto rect = label->GetComponent<UnityEngine::RectTransform*>();
        rect->anchorMin = {0.0f, 1.0f};
        rect->anchorMax = {0.0f, 1.0f};
        rect->pivot = {0.0f, 1.0f};
        rect->sizeDelta = {pitch, height};
        return rect;
    }

    void Apply() {
        bool hasLeft = offset > 0;
        int visible = slots - (hasLeft ? 1 : 0);
        bool hasRight = offset + visible < count;
        if (hasRight) {
            visible -= 1;
        }
        int start = hasLeft ? 1 : 0;

        auto world = bar->TransformPoint({originX + start * pitch, top, 0.0f});
        viewport->position = world;
        auto size = viewport->parent.cast<UnityEngine::RectTransform>()->InverseTransformVector(
            bar->TransformVector({visible * pitch, height, 0.0f})
        );
        viewport->sizeDelta = {std::abs(size.x), std::abs(size.y)};

        auto shift = bar->TransformVector({(start - offset) * pitch, 0.0f, 0.0f});
        control->position = {baseWorld.x + shift.x, baseWorld.y + shift.y, baseWorld.z + shift.z};

        leftArrow->SetActive(hasLeft);
        rightArrow->SetActive(hasRight);
        Logger.info("Tabs {} offset {} visible {} left {} right {}", count, offset, visible, hasLeft, hasRight);
    }

    int MaxOffset() {
        return std::max(0, count - (slots - 1));
    }

    void Next() {
        bool hasLeft = offset > 0;
        int visible = slots - (hasLeft ? 1 : 0);
        if (offset + visible < count) {
            visible -= 1;
        }
        offset = std::min(offset + visible, MaxOffset());
        Apply();
    }

    void Previous() {
        offset = std::max(0, offset - (slots - 1));
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
        auto first = iconControl->_cells->get_Item(0)->GetComponent<UnityEngine::RectTransform*>();
        auto second = iconControl->_cells->get_Item(1)->GetComponent<UnityEngine::RectTransform*>();
        auto firstLeft = BarPoint(first, 1);
        auto secondLeft = BarPoint(second, 1);
        auto firstBottom = BarPoint(first, 0);
        pitch = secondLeft.x - firstLeft.x;
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
            viewport = rect;
            baseWorld = controlRect->position;
            auto size = controlRect->sizeDelta;
            controlRect->SetParent(rect, true);
            controlRect->position = baseWorld;
            controlRect->sizeDelta = size;
            control = controlRect;
            Logger.info("Viewport created, pitch {} height {} slots {} tabs {}", pitch, height, slots, count);
        } else {
            control = controlRect;
        }

        if (!leftArrow) {
            auto left = MakeArrow("<", [] { Previous(); });
            left->anchoredPosition = {originX, top};
            leftArrow = left->gameObject;
        }
        if (!rightArrow) {
            auto right = MakeArrow(">", [] { Next(); });
            right->anchoredPosition = {originX + (slots - 1) * pitch, top};
            rightArrow = right->gameObject;
        }
        offset = std::min(offset, MaxOffset());
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
        Logger.warn("Pager failed: {}", e.what());
    }
}
