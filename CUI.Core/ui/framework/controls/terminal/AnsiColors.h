#pragma once
#include "CellData.h"
#include "TerminalTheme.h"

namespace CUI {
namespace Term {

class AnsiColors {
public:
    explicit AnsiColors(const TerminalTheme& theme = TerminalTheme::Dark());

    TermColor DefaultForeground() const { return m_theme.Foreground; }
    TermColor DefaultBackground() const { return m_theme.Background; }
    TermColor SelectionBackground() const { return m_theme.Selection; }
    TermColor SelectionForeground() const;
    TermColor CursorColor() const { return m_theme.Cursor; }

    const TerminalTheme& Theme() const { return m_theme; }

    void SetTheme(const TerminalTheme& theme);

    TermColor Resolve(int color, bool foreground) const;
    TermColor Palette(int index) const;
    void SetPalette(int index, TermColor color);

private:
    void RebuildTable();
    static TermColor ContrastingForeground(TermColor bg);

    TerminalTheme m_theme;
    TermColor m_table256[256];
};

} // namespace Term
} // namespace CUI
