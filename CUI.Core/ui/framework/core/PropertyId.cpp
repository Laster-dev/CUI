#include "PropertyId.h"
#include <cstring>

namespace CUI {

namespace {

constexpr const char* kPropertyNamesById[] = {
    "",
    "width", "height", "minWidth", "minHeight", "maxWidth", "maxHeight", "margin", "padding",
    "visibility", "isEnabled", "opacity", "cornerRadius", "borderThickness",
    "flexGrow", "align", "alignHorizontal", "alignVertical", "orientation", "gap",
    "itemWidth", "itemHeight", "lastChildFill", "justifyLines", "fillLastLine", "rows", "columns", "clipToBounds",
    "Canvas.Left", "Canvas.Top", "Canvas.Right", "Canvas.Bottom", "Canvas.ZIndex",
    "Grid.Column", "Grid.Row", "Grid.ColumnSpan", "Grid.RowSpan", "DockPanel.Dock",
    "text", "placeholder", "fontFamily", "fontSize", "fontWeight", "fontStyle", "fontStretch", "isUnderline", "isStrikethrough", "toolTip", "icon", "focused",
    "theme.backgroundToken", "theme.hoverBackgroundToken", "theme.pressedBackgroundToken",
    "theme.disabledBackgroundToken", "theme.borderToken", "theme.focusedBorderToken", "theme.colorToken",
    "theme.secondaryColorToken", "theme.placeholderColorToken", "theme.selectedBackgroundToken",
    "theme.headerBackgroundToken", "theme.paneBackgroundToken", "theme.indicatorColorToken",
    "theme.dropdownBackgroundToken", "theme.selectedItemBackgroundToken", "theme.fillColorToken",
    "theme.trackColorToken", "theme.activeTrackColorToken", "theme.thumbColorToken",
    "theme.onColorToken", "theme.offColorToken", "theme.knobColorToken", "theme.checkedBackgroundToken",
    "theme.accentColorToken", "theme.activeColorToken", "theme.underlineColorToken",
    "theme.activeUnderlineColorToken", "theme.activeTabBackgroundToken", "theme.inactiveTabBackgroundToken",
    "theme.gridLineBrushToken", "theme.titleColorToken", "theme.messageColorToken", "theme.caretColorToken",
    "background", "borderBrush", "hoverBackground", "pressedBackground", "color",
    "items", "value", "checkState", "isThreeState", "isIndeterminate", "isOn",
    "dateStr", "timeStr", "selectedColor", "currentPage", "totalPages",
    "minimum", "maximum", "step",
    "isReadOnly", "isClearEnabled",
    "lowerValue", "upperValue", "minimumRange",
    "header", "subtitle", "isExpanded", "expandDirection", "groupName",
    "lineSpacing", "lineHeight", "caretWidth", "caretBlinkRate",
    "textWrapping", "acceptsReturn", "isPasswordRevealed", "showRevealButton",
    "showLineNumbers", "indentWidth", "minTabWidth", "maxTabWidth",
    "paneTitle", "openPaneLength", "compactPaneLength", "isPaneOpen",
    "title", "message", "navigateUri", "source", "stretch", "severity",
    "isOpen", "isClosable", "actionText", "selectedIndex", "shell",
    "followTail", "maxEntries", "showGrid", "showLegend", "showTooltip",
    "labelPosition", "durationMs", "autoClose", "closeable",
    "offsetX", "offsetY", "spacing", "corner",
};

static_assert(sizeof(kPropertyNamesById) / sizeof(kPropertyNamesById[0]) ==
              static_cast<size_t>(PropertyId::Count),
              "PropertyId name table out of sync with enum");

PropertyId AliasFromName(const char* name) {
    if (!name) return PropertyId::None;
    if (std::strcmp(name, "theme.accentToken") == 0) return PropertyId::AccentColorToken;
    if (std::strcmp(name, "TextWrapping") == 0) return PropertyId::TextWrapping;
    if (std::strcmp(name, "AcceptsReturn") == 0) return PropertyId::AcceptsReturn;
    return PropertyId::None;
}

} // namespace

const char* PropertyIdToName(PropertyId id) {
    const auto idx = static_cast<uint16_t>(id);
    if (idx >= static_cast<uint16_t>(PropertyId::Count)) return "";
    return kPropertyNamesById[idx];
}

PropertyId PropertyIdFromName(const char* name) {
    if (!name || !name[0]) return PropertyId::None;
    if (PropertyId alias = AliasFromName(name); alias != PropertyId::None) return alias;
    for (uint16_t i = 1; i < static_cast<uint16_t>(PropertyId::Count); ++i) {
        if (std::strcmp(kPropertyNamesById[i], name) == 0) {
            return static_cast<PropertyId>(i);
        }
    }
    return PropertyId::None;
}

PropertyId PropertyIdFromName(const std::string& name) {
    return PropertyIdFromName(name.c_str());
}

} // namespace CUI
