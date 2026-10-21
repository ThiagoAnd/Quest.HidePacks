#include "autohooks/shared/hooks.hpp"
#include "logger.hpp"

#include <string>

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

}

MAKE_LATE_HOOK_MATCH(
    Selectable_Evaluate,
    &UnityEngine::UI::Selectable::EvaluateAndTransitionToSelectionState,
    void,
    UnityEngine::UI::Selectable* self
) {
    Selectable_Evaluate(self);
    try {
        auto path = PathOf(self->transform);
        if (path.find("PanelViewController") != std::string::npos) {
            Logger.info("State {}", path);
        }
    } catch (std::exception const& e) {
        Logger.warn("Trace failed: {}", e.what());
    }
}

MAKE_LATE_HOOK_MATCH(
    Button_Press,
    &UnityEngine::UI::Button::Press,
    void,
    UnityEngine::UI::Button* self
) {
    Button_Press(self);
    try {
        Logger.info("Press {}", PathOf(self->transform));
    } catch (std::exception const& e) {
        Logger.warn("Trace failed: {}", e.what());
    }
}
