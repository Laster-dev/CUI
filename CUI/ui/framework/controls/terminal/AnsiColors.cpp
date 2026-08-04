#include "AnsiColors.h"

namespace CUI {
namespace Term {

AnsiColors::AnsiColors(const TerminalTheme& theme)
    : m_theme(theme) {
    RebuildTable();
}

TermColor AnsiColors::SelectionForeground() const {
    if (m_theme.SelectionForeground.a != 0) {
        return m_theme.SelectionForeground;
    }
    return ContrastingForeground(SelectionBackground());
}

void AnsiColors::SetTheme(const TerminalTheme& theme) {
    m_theme = theme;
    RebuildTable();
}

TermColor AnsiColors::Resolve(int color, bool foreground) const {
    if (color == CellData::DefaultColor) {
        return foreground ? DefaultForeground() : DefaultBackground();
    }

    if ((color & CellData::ColorModeRgb) != 0) {
        const int rgb = color & 0xFFFFFF;
        return TermColor::FromRgb(
            static_cast<uint8_t>((rgb >> 16) & 0xFF),
            static_cast<uint8_t>((rgb >> 8) & 0xFF),
            static_cast<uint8_t>(rgb & 0xFF));
    }

    if (color >= 0 && color < 256) {
        return m_table256[color];
    }

    return foreground ? DefaultForeground() : DefaultBackground();
}

TermColor AnsiColors::Palette(int index) const {
    if (index < 0 || index > 255) {
        return DefaultForeground();
    }
    return m_table256[index];
}

void AnsiColors::SetPalette(int index, TermColor color) {
    if (index < 0 || index > 255) {
        return;
    }
    m_table256[index] = color;
    if (index < 16) {
        m_theme.Ansi16[index] = color;
    }
}

void AnsiColors::RebuildTable() {
    for (int i = 0; i < 16; ++i) {
        m_table256[i] = m_theme.Ansi16[i];
    }

    int index = 16;
    const int values[6] = { 0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF };
    for (int r = 0; r < 6; ++r) {
        for (int g = 0; g < 6; ++g) {
            for (int b = 0; b < 6; ++b) {
                m_table256[index++] = TermColor::FromRgb(
                    static_cast<uint8_t>(values[r]),
                    static_cast<uint8_t>(values[g]),
                    static_cast<uint8_t>(values[b]));
            }
        }
    }

    for (int i = 0; i < 24; ++i) {
        const uint8_t v = static_cast<uint8_t>(8 + i * 10);
        m_table256[232 + i] = TermColor::FromRgb(v, v, v);
    }
}

TermColor AnsiColors::ContrastingForeground(TermColor bg) {
    const double lum = 0.299 * bg.r + 0.587 * bg.g + 0.114 * bg.b;
    return lum > 160.0
        ? TermColor::FromRgb(0x1E, 0x1E, 0x1E)
        : TermColor::FromRgb(0xF2, 0xF2, 0xF2);
}

} // namespace Term
} // namespace CUI
