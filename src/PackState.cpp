#include "PackState.hpp"

#include <unordered_set>

#include "Config.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Time.hpp"
#include "logger.hpp"

namespace {
    constexpr float MessageSeconds = 3.0f;

    std::vector<PackState::Entry> entries;
    std::vector<std::string> builtIds;
    std::unordered_set<std::string> pending;
    std::vector<UnityW<BSML::ToggleSetting>> toggles;
    std::vector<UnityW<UnityEngine::Transform>> rows;
    UnityW<TMPro::TextMeshProUGUI> counter;
    UnityW<UnityEngine::UI::Button> applyButton;
    bool syncing = false;
    bool blocked = false;
    bool messageShown = false;
    float messageUntil = 0.0f;

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
        PackState::Refresh();
    }
}

namespace PackState {
    std::vector<Entry> const& Entries() {
        return entries;
    }

    void Load(GlobalNamespace::LevelFilteringNavigationController* navigation) {
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
                    entry.id = Text(pack->packID);
                    entry.name = Text(pack->packName);
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
            ReportError("could not read the packs", e);
        }
        if (!found.empty()) {
            entries = std::move(found);
        }
        Logger.info("Panel has {} packs, {} without cover", entries.size(), missing);
    }

    void BeginSession() {
        toggles.clear();
        rows.clear();
        pending = Config::Hidden();
    }

    void MarkBuilt() {
        builtIds.clear();
        for (auto const& entry : entries) {
            builtIds.push_back(entry.id);
        }
    }

    bool RowsStale() {
        if (builtIds.size() != entries.size()) {
            return true;
        }
        for (size_t i = 0; i < entries.size(); i++) {
            if (builtIds[i] != entries[i].id) {
                return true;
            }
        }
        return false;
    }

    bool IsVisible(std::string const& id) {
        return !pending.contains(id);
    }

    void AddToggle(BSML::ToggleSetting* toggle) {
        toggles.push_back(toggle);
    }

    void AddRow(UnityEngine::Transform* row) {
        rows.push_back(row);
    }

    void BindCounter(TMPro::TextMeshProUGUI* text) {
        counter = text;
    }

    void BindApply(UnityEngine::UI::Button* button) {
        applyButton = button;
    }

    void OnSwitch(std::string const& id, bool visible) {
        if (syncing) {
            return;
        }
        if (visible) {
            pending.erase(id);
        } else {
            pending.insert(id);
        }
        blocked = false;
        messageShown = false;
        Refresh();
    }

    void Refresh() {
        if (!counter) {
            return;
        }
        int hidden = HiddenCount();
        int total = static_cast<int>(entries.size());
        if (applyButton) {
            applyButton->interactable = pending != Config::Hidden();
        }
        if (blocked && hidden >= total) {
            counter->text = "Keep 1 visible";
            return;
        }
        if (messageShown) {
            counter->text = "Applied: " + std::to_string(hidden) + " hidden";
            return;
        }
        counter->text = std::to_string(hidden) + "/" + std::to_string(total) + " hidden";
    }

    void Reset() {
        pending = Config::Hidden();
        blocked = false;
        messageShown = false;
        SyncToggles();
    }

    void ShowAll() {
        pending.clear();
        blocked = false;
        messageShown = false;
        SyncToggles();
    }

    void Apply() {
        if (!entries.empty() && HiddenCount() >= static_cast<int>(entries.size())) {
            blocked = true;
            Refresh();
            return;
        }
        blocked = false;
        Config::SetHidden(pending);
        Config::Save();
        messageShown = true;
        messageUntil = UnityEngine::Time::get_realtimeSinceStartup() + MessageSeconds;
        Refresh();
    }

    void FilterRows(bool onlyDlc) {
        for (size_t i = 0; i < rows.size() && i < entries.size(); i++) {
            if (rows[i]) {
                rows[i]->gameObject->SetActive(!onlyDlc || entries[i].dlc);
            }
        }
    }

    void Tick() {
        if (messageShown && UnityEngine::Time::get_realtimeSinceStartup() >= messageUntil) {
            messageShown = false;
            Refresh();
        }
    }
}
