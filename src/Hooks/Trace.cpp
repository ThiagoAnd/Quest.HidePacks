#include "autohooks/shared/hooks.hpp"
#include "logger.hpp"

#include <string>

#include "UnityEngine/EventSystems/PointerEventData.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/Selectable.hpp"

namespace {
    std::string PathOf(UnityEngine::Transform* transform) {
        std::string path;
        int depth = 0;
        while (transform && depth < 8) {
            path = static_cast<std::string>(transform->name) + "/" + path;
            transform = transform->parent;
            depth++;
        }
        return path;
    }

    std::string Describe(UnityEngine::EventSystems::PointerEventData* data) {
        if (!data) {
            return "none";
        }
        auto hit = data->pointerCurrentRaycast.get_gameObject();
        return hit ? PathOf(hit->transform) : "nothing";
    }
}

MAKE_LATE_HOOK_MATCH(
    Selectable_OnPointerEnter,
    &UnityEngine::UI::Selectable::OnPointerEnter,
    void,
    UnityEngine::UI::Selectable* self,
    UnityEngine::EventSystems::PointerEventData* data
) {
    Selectable_OnPointerEnter(self, data);
    try {
        auto path = PathOf(self->transform);
        if (path.find("PanelViewController") != std::string::npos) {
            Logger.info("Enter {} hit {}", path, Describe(data));
        }
    } catch (std::exception const& e) {
        Logger.warn("Trace failed: {}", e.what());
    }
}

MAKE_LATE_HOOK_MATCH(
    Button_OnPointerClick,
    &UnityEngine::UI::Button::OnPointerClick,
    void,
    UnityEngine::UI::Button* self,
    UnityEngine::EventSystems::PointerEventData* data
) {
    Button_OnPointerClick(self, data);
    try {
        Logger.info("Click {} hit {}", PathOf(self->transform), Describe(data));
    } catch (std::exception const& e) {
        Logger.warn("Trace failed: {}", e.what());
    }
}
