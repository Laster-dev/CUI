#pragma once
#include <cstdint>
#include <string>

namespace CUI {

// Stable theme color ids — paint path never hashes token name strings.
enum class ThemeTokenId : uint16_t {
    Unset = 0,
    WindowBackground,
    CardBackground,
    CardBorder,
    TextPrimary,
    TextSecondary,
    TextMuted,
    TitleBarBackground,
    TitleBarText,
    AccentColor,
    AccentForeground,
    DangerColor,
    PaneBackground,
    InputBackground,
    InputBorder,
    HoverBackground,
    PressedBackground,
    SelectedBackground,
    FocusedBorder,
    ActivityBarBackground,
    SideBarBackground,
    EditorBackground,
    StatusBarBackground,
    TabBarBackground,
    Count
};

const char* ThemeTokenIdToName(ThemeTokenId id);
ThemeTokenId ThemeTokenIdFromName(const std::string& name);
ThemeTokenId ThemeTokenIdFromName(const char* name);

} // namespace CUI
