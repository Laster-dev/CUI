#include "ThemeTokenId.h"
#include <cstring>

namespace CUI {

namespace {
// Cold-path labels only (PropertyGrid / legacy SetProperty bridge).
// Paint path uses ThemeTokenId directly — never indexes this table per frame.
constexpr const char* kTokenNamesById[] = {
    "", // Unset
    "windowBackground",
    "cardBackground",
    "cardBorder",
    "textPrimary",
    "textSecondary",
    "textMuted",
    "accentColor",
    "accentForeground",
    "dangerColor",
    "paneBackground",
    "inputBackground",
    "inputBorder",
    "hoverBackground",
    "pressedBackground",
    "selectedBackground",
    "focusedBorder",
};
static_assert(sizeof(kTokenNamesById) / sizeof(kTokenNamesById[0]) ==
              static_cast<size_t>(ThemeTokenId::Count),
              "ThemeTokenId name table out of sync with enum");
} // namespace

const char* ThemeTokenIdToName(ThemeTokenId id) {
    const auto idx = static_cast<uint16_t>(id);
    if (idx >= static_cast<uint16_t>(ThemeTokenId::Count)) {
        return "";
    }
    return kTokenNamesById[idx];
}

ThemeTokenId ThemeTokenIdFromName(const char* name) {
    if (!name || !name[0]) {
        return ThemeTokenId::Unset;
    }
    for (uint16_t i = 1; i < static_cast<uint16_t>(ThemeTokenId::Count); ++i) {
        if (std::strcmp(kTokenNamesById[i], name) == 0) {
            return static_cast<ThemeTokenId>(i);
        }
    }
    return ThemeTokenId::Unset;
}

ThemeTokenId ThemeTokenIdFromName(const std::string& name) {
    return ThemeTokenIdFromName(name.c_str());
}

} // namespace CUI
