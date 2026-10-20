#pragma once

#include "scotland2/shared/loader.hpp"

#if __has_include("git-info.h")
#include "git-info.h"
#endif

#ifdef GIT_VERSION
inline modloader::ModInfo const modInfo{MOD_ID, GIT_VERSION, 0};
#else
inline modloader::ModInfo const modInfo{MOD_ID, VERSION, 0};
#endif
