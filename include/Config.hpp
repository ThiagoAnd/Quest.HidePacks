#pragma once

#include <string>
#include <string_view>
#include <unordered_set>

namespace Config {
    void Load();
    void Save();
    bool IsHidden(std::string_view packId);
    const std::unordered_set<std::string>& Hidden();
    void SetHidden(std::unordered_set<std::string> hidden);
}
