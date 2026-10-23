#include "main.hpp"

#include "Config.hpp"
#include "Tab.hpp"
#include "autohooks/shared/hooks.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "custom-types/shared/register.hpp"
#include "logger.hpp"
#include "modInfo.hpp"
#include "UnityEngine/Application.hpp"
#include "scotland2/shared/modloader.h"

MOD_EXPORT_FUNC void setup(CModInfo& info) {
    Paper::Logger::RegisterFileContextId(MOD_ID);
    info = modInfo.to_c();
    Logger.info("Setup done");
}

MOD_EXPORT_FUNC void load() {
    il2cpp_functions::Init();

    auto earlyHookCount = EARLY_HOOK_COUNT;
    if (earlyHookCount > 0) {
        Logger.info("Installing {} early hooks", earlyHookCount);
        INSTALL_EARLY_HOOKS();
    }
}

MOD_EXPORT_FUNC void late_load() {
    Logger.info("===== HidePacks {} starting on Beat Saber {} =====", VERSION, Text(UnityEngine::Application::get_version()));
    Config::Load();
    Tab::Register();
    custom_types::Register::AutoRegister();
    auto lateHookCount = LATE_HOOK_COUNT;
    if (lateHookCount > 0) {
        Logger.info("Installing {} late hooks", lateHookCount);
        INSTALL_LATE_HOOKS();
        Logger.info("Late hooks installed");
    }
}
