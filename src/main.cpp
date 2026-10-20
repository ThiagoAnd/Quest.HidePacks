#include "main.hpp"

#include "Config.hpp"
#include "autohooks/shared/hooks.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "logger.hpp"
#include "modInfo.hpp"
#include "scotland2/shared/modloader.h"

MOD_EXPORT_FUNC void setup(CModInfo& info) {
    info = modInfo.to_c();
    Logger.info("Setup done");
}

MOD_EXPORT_FUNC void load() {
    il2cpp_functions::Init();
    Config::Load();

    auto earlyHookCount = EARLY_HOOK_COUNT;
    if (earlyHookCount > 0) {
        Logger.info("Installing {} early hooks", earlyHookCount);
        INSTALL_EARLY_HOOKS();
    }
}

MOD_EXPORT_FUNC void late_load() {
    auto lateHookCount = LATE_HOOK_COUNT;
    if (lateHookCount > 0) {
        Logger.info("Installing {} late hooks", lateHookCount);
        INSTALL_LATE_HOOKS();
        Logger.info("Late hooks installed");
    }
}
