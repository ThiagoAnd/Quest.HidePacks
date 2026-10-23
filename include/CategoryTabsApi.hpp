#pragma once

#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>

#include "scotland2/shared/modloader.h"

extern "C" {
    typedef void (*CategoryTabsSelected)(void* navigation);
    typedef void (*CategoryTabsDeselected)();

    typedef struct {
        char const* id;
        char const* tooltip;
        uint8_t const* icon;
        size_t iconSize;
        CategoryTabsSelected onSelected;
        CategoryTabsDeselected onDeselected;
    } CategoryTabsTab;
}

namespace CategoryTabsApi {
    inline constexpr uint32_t Version = 1;

    inline void* Find(char const* name) {
        CModInfo info{"CategoryTabs", "", 0};
        auto found = modloader_get_mod(&info, MatchType_IdOnly);
        if (!found.handle) {
            return nullptr;
        }
        return dlsym(found.handle, name);
    }

    inline bool Available() {
        auto version = reinterpret_cast<uint32_t (*)()>(Find("CategoryTabs_ApiVersion"));
        return version && version() == Version;
    }

    inline bool Register(CategoryTabsTab const& tab) {
        auto add = reinterpret_cast<bool (*)(CategoryTabsTab const*)>(Find("CategoryTabs_Register"));
        return Available() && add && add(&tab);
    }
}
