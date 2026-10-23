#include "autohooks/shared/hooks.hpp"
#include "logger.hpp"

#include <string>
#include <vector>

#include "Config.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "Tab.hpp"
#include "System/Collections/Generic/IReadOnlyCollection_1.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Collections/Generic/List_1.hpp"

using namespace GlobalNamespace;

namespace {
    using PackList = System::Collections::Generic::IReadOnlyList_1<BeatmapLevelPack*>;

    constexpr std::string_view CustomPrefix = "custom_levelPack_";

    bool IsCustom(std::string const& id) {
        return id.rfind(CustomPrefix, 0) == 0;
    }

    PackList* Filter(PackList* source) {
        int count = source->i___System__Collections__Generic__IReadOnlyCollection_1_T_()->Count;
        std::vector<BeatmapLevelPack*> kept;
        for (int i = 0; i < count; i++) {
            auto pack = source->get_Item(i);
            if (!pack) {
                return source;
            }
            std::string id = Text(pack->packID);
            if (!IsCustom(id) && Config::IsHidden(id)) {
                continue;
            }
            kept.push_back(pack);
        }
        if (kept.size() == static_cast<size_t>(count)) {
            return source;
        }
        if (kept.empty()) {
            Logger.warn("Every pack is hidden, keeping the full list");
            return source;
        }
        auto list = System::Collections::Generic::List_1<BeatmapLevelPack*>::New_ctor();
        for (auto pack : kept) {
            list->Add(pack);
        }
        Logger.info("Hid {} of {} packs", count - static_cast<int>(kept.size()), count);
        return il2cpp_utils::cast<PackList>(list);
    }
}

MAKE_LATE_HOOK_MATCH(
    LevelFilteringNavigationController_ShowPacksInSecondChildController,
    &LevelFilteringNavigationController::ShowPacksInSecondChildController,
    void,
    LevelFilteringNavigationController* self,
    PackList* beatmapLevelPacks
) {
    auto packs = beatmapLevelPacks;
    if (packs && Tab::Registered()) {
        try {
            packs = Filter(beatmapLevelPacks);
        } catch (std::exception const& e) {
            ReportError("filter failed", e);
            packs = beatmapLevelPacks;
        }
    }
    LevelFilteringNavigationController_ShowPacksInSecondChildController(self, packs);
}
