#pragma once

#include <exception>
#include <string>

#include "paper2_scotland2/shared/logger.hpp"

inline constexpr auto Logger = Paper::ConstLoggerContext(MOD_ID);

inline void ReportError(std::string const& where, std::exception const& e) {
    Logger.error("HIDEPACKS ERROR in {}: {}. Please send this file when reporting it.", where, e.what());
}
