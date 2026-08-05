#!/usr/bin/env python3
"""Bulk-replace common SetProperty("...") patterns with typed setters."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "CUI"

TOKEN_MAP = {
    "windowBackground": "WindowBackground",
    "cardBackground": "CardBackground",
    "cardBorder": "CardBorder",
    "textPrimary": "TextPrimary",
    "textSecondary": "TextSecondary",
    "textMuted": "TextMuted",
    "titleBarBackground": "TitleBarBackground",
    "titleBarText": "TitleBarText",
    "accentColor": "AccentColor",
    "accentForeground": "AccentForeground",
    "dangerColor": "DangerColor",
    "paneBackground": "PaneBackground",
    "inputBackground": "InputBackground",
    "inputBorder": "InputBorder",
    "hoverBackground": "HoverBackground",
    "pressedBackground": "PressedBackground",
    "selectedBackground": "SelectedBackground",
    "focusedBorder": "FocusedBorder",
    "activityBarBackground": "ActivityBarBackground",
    "sideBarBackground": "SideBarBackground",
    "editorBackground": "EditorBackground",
    "statusBarBackground": "StatusBarBackground",
    "tabBarBackground": "TabBarBackground",
}

TOKEN_SETTERS = {
    "theme.backgroundToken": "SetBackgroundToken",
    "theme.hoverBackgroundToken": "SetHoverBackgroundToken",
    "theme.pressedBackgroundToken": "SetPressedBackgroundToken",
    "theme.disabledBackgroundToken": "SetDisabledBackgroundToken",
    "theme.borderToken": "SetBorderToken",
    "theme.focusedBorderToken": "SetFocusedBorderToken",
    "theme.colorToken": "SetColorToken",
    "theme.secondaryColorToken": "SetSecondaryColorToken",
    "theme.placeholderColorToken": "SetPlaceholderColorToken",
    "theme.selectedBackgroundToken": "SetSelectedBackgroundToken",
    "theme.headerBackgroundToken": "SetHeaderBackgroundToken",
    "theme.paneBackgroundToken": "SetPaneBackgroundToken",
    "theme.indicatorColorToken": "SetIndicatorColorToken",
    "theme.dropdownBackgroundToken": "SetDropdownBackgroundToken",
    "theme.selectedItemBackgroundToken": "SetSelectedItemBackgroundToken",
    "theme.fillColorToken": "SetFillColorToken",
    "theme.trackColorToken": "SetTrackColorToken",
    "theme.activeTrackColorToken": "SetActiveTrackColorToken",
    "theme.thumbColorToken": "SetThumbColorToken",
    "theme.onColorToken": "SetOnColorToken",
    "theme.offColorToken": "SetOffColorToken",
    "theme.knobColorToken": "SetKnobColorToken",
    "theme.checkedBackgroundToken": "SetCheckedBackgroundToken",
    "theme.accentColorToken": "SetAccentColorToken",
    "theme.accentToken": "SetAccentColorToken",
    "theme.activeColorToken": "SetActiveColorToken",
    "theme.underlineColorToken": "SetUnderlineColorToken",
    "theme.activeUnderlineColorToken": "SetActiveUnderlineColorToken",
    "theme.activeTabBackgroundToken": "SetActiveTabBackgroundToken",
    "theme.inactiveTabBackgroundToken": "SetInactiveTabBackgroundToken",
    "theme.gridLineBrushToken": "SetGridLineBrushToken",
    "theme.titleColorToken": "SetTitleColorToken",
    "theme.messageColorToken": "SetMessageColorToken",
    "theme.caretColorToken": "SetCaretColorToken",
}

SIMPLE_SETTERS = {
    "width": "SetWidth",
    "height": "SetHeight",
    "minWidth": "SetMinWidth",
    "minHeight": "SetMinHeight",
    "opacity": "SetOpacity",
    "cornerRadius": "SetCornerRadius",
    "borderThickness": "SetBorderThickness",
    "flexGrow": "SetFlexGrow",
    "gap": "SetGap",
    "itemWidth": "SetItemWidth",
    "itemHeight": "SetItemHeight",
    "rows": "SetRows",
    "columns": "SetColumns",
    "fontSize": "SetFontSize",
    "Canvas.Left": "SetCanvasLeft",
    "Canvas.Top": "SetCanvasTop",
    "Canvas.Right": "SetCanvasRight",
    "Canvas.Bottom": "SetCanvasBottom",
    "Grid.Column": "SetGridColumn",
    "Grid.Row": "SetGridRow",
    "Grid.ColumnSpan": "SetGridColumnSpan",
    "Grid.RowSpan": "SetGridRowSpan",
}

STRING_SETTERS = {
    "text": "SetText",
    "placeholder": "SetPlaceholder",
    "fontFamily": "SetFontFamily",
    "fontWeight": "SetFontWeight",
    "toolTip": "SetToolTip",
}

COLOR_SETTERS = {
    "background": "SetBackground",
    "hoverBackground": "SetHoverBackground",
    "pressedBackground": "SetPressedBackground",
    "borderBrush": "SetBorderBrush",
    "color": "SetColor",
}

ORIENT_MAP = {
    "Horizontal": "Orientation::Horizontal",
    "Vertical": "Orientation::Vertical",
    "Row": "Orientation::Horizontal",
}

ALIGN_MAP = {
    "Stretch": "Alignment::Stretch",
    "Start": "Alignment::Start",
    "Center": "Alignment::Center",
    "End": "Alignment::End",
    "Left": "Alignment::Start",
    "Right": "Alignment::End",
    "Top": "Alignment::Start",
    "Bottom": "Alignment::End",
}

DOCK_MAP = {
    "Left": "Dock::Left",
    "Right": "Dock::Right",
    "Top": "Dock::Top",
    "Bottom": "Dock::Bottom",
}

# Match: expr->SetProperty("name", Value(...)); or SetProperty("name", Value(...));
# Also CUI::Value
CALL_RE = re.compile(
    r'''(?P<prefix>(?:[\w:>.\-\>\(\)\s]*?)?)'''
    r'''SetProperty\(\s*"(?P<name>[^"]+)"\s*,\s*(?:CUI::)?Value\((?P<arg>.*)\)\s*\)'''
)


def unwrap_value_arg(arg: str) -> str:
    return arg.strip()


def token_enum(name: str) -> str | None:
    enum = TOKEN_MAP.get(name)
    if not enum:
        return None
    return f"ThemeTokenId::{enum}"


def transform_call(prefix: str, name: str, arg: str) -> str | None:
    arg = unwrap_value_arg(arg)
    p = prefix or ""

    if name in TOKEN_SETTERS:
        # Value("tokenName") or Value(var)
        m = re.fullmatch(r'"([^"]*)"', arg)
        if m:
            te = token_enum(m.group(1))
            if te:
                return f"{p}{TOKEN_SETTERS[name]}({te})"
            return f"{p}{TOKEN_SETTERS[name]}(ThemeTokenIdFromName({arg}))"
        return f"{p}{TOKEN_SETTERS[name]}(ThemeTokenIdFromName({arg}))"

    if name in SIMPLE_SETTERS:
        return f"{p}{SIMPLE_SETTERS[name]}({arg})"

    if name in STRING_SETTERS:
        return f"{p}{STRING_SETTERS[name]}({arg})"

    if name == "padding":
        return f"{p}SetPadding({arg})"
    if name == "margin":
        return f"{p}SetMargin({arg})"
    if name == "clipToBounds":
        return f"{p}SetClipToBounds({arg})"
    if name == "lastChildFill":
        return f"{p}SetLastChildFill({arg})"
    if name == "isEnabled":
        return f"{p}SetIsEnabled({arg})"

    if name == "orientation":
        m = re.fullmatch(r'"([^"]*)"', arg)
        if m and m.group(1) in ORIENT_MAP:
            return f"{p}SetOrientation({ORIENT_MAP[m.group(1)]})"
        return None

    if name == "align":
        m = re.fullmatch(r'"([^"]*)"', arg)
        if m and m.group(1) in ALIGN_MAP:
            return f"{p}SetAlign({ALIGN_MAP[m.group(1)]})"
        return None

    if name == "alignHorizontal":
        m = re.fullmatch(r'"([^"]*)"', arg)
        if m and m.group(1) in ALIGN_MAP:
            return f"{p}SetAlignHorizontal({ALIGN_MAP[m.group(1)]})"
        return None

    if name == "alignVertical":
        m = re.fullmatch(r'"([^"]*)"', arg)
        if m and m.group(1) in ALIGN_MAP:
            return f"{p}SetAlignVertical({ALIGN_MAP[m.group(1)]})"
        return None

    if name == "DockPanel.Dock":
        m = re.fullmatch(r'"([^"]*)"', arg)
        if m and m.group(1) in DOCK_MAP:
            return f"{p}SetDock({DOCK_MAP[m.group(1)]})"
        return None

    if name == "visibility":
        m = re.fullmatch(r'"([^"]*)"', arg)
        if m:
            return f"{p}SetVisibility(Visibility::{m.group(1)})"
        return None

    if name in COLOR_SETTERS:
        # Value(ThemeManager::Instance().GetColor("x")) -> keep as GetColor arg
        # Value(D2D1::ColorF(...)) -> pass through
        # Value(tokens.foo) -> pass through
        # Value(colorStr) string - leave for manual (needs parse)
        m = re.fullmatch(r'"([^"]*)"', arg)
        if m:
            return None  # hex/string color — manual
        return f"{p}{COLOR_SETTERS[name]}({arg})"

    return None


def process_text(text: str) -> tuple[str, int]:
    count = 0

    def repl(m: re.Match) -> str:
        nonlocal count
        result = transform_call(m.group("prefix"), m.group("name"), m.group("arg"))
        if result is None:
            return m.group(0)
        count += 1
        return result

    # Multi-pass for nested / sequential; process line-oriented to keep arg matching sane
    out_lines = []
    for line in text.splitlines(keepends=True):
        if 'SetProperty("' not in line and "SetProperty('" not in line:
            out_lines.append(line)
            continue
        # Handle multiple SetProperty on one line
        new_line = line
        while True:
            m = CALL_RE.search(new_line)
            if not m:
                break
            result = transform_call(m.group("prefix"), m.group("name"), m.group("arg"))
            if result is None:
                # skip this match by advancing
                # replace with placeholder then restore — just break to avoid infinite loop
                # Try next occurrence after this one
                start = m.end()
                rest = new_line[start:]
                m2 = CALL_RE.search(rest)
                if not m2:
                    break
                # Only transform later ones if earlier failed — rare
                break
            count += 1
            new_line = new_line[: m.start()] + result + new_line[m.end() :]
        out_lines.append(new_line)
    return "".join(out_lines), count


def iter_files():
    for base in (ROOT / "ui", ROOT / "showcase"):
        if not base.exists():
            continue
        for p in base.rglob("*"):
            if p.suffix.lower() in {".cpp", ".h", ".hpp"}:
                yield p


def main():
    dry = "--dry" in sys.argv
    total = 0
    files_changed = 0
    for path in iter_files():
        text = path.read_text(encoding="utf-8", errors="replace")
        if 'SetProperty("' not in text:
            continue
        new_text, n = process_text(text)
        if n == 0 or new_text == text:
            continue
        total += n
        files_changed += 1
        print(f"{path.relative_to(ROOT)}: {n}")
        if not dry:
            path.write_text(new_text, encoding="utf-8")
    print(f"TOTAL replacements: {total} in {files_changed} files" + (" (dry)" if dry else ""))


if __name__ == "__main__":
    main()
