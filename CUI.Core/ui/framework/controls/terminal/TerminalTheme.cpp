#include "TerminalTheme.h"

namespace CUI {
namespace Term {

TerminalTheme::TerminalTheme() {
    Ansi16[0] = TermColor::FromRgb(0x0C, 0x0C, 0x0C);
    Ansi16[1] = TermColor::FromRgb(0xC5, 0x0F, 0x1F);
    Ansi16[2] = TermColor::FromRgb(0x13, 0xA1, 0x0E);
    Ansi16[3] = TermColor::FromRgb(0xC1, 0x9C, 0x00);
    Ansi16[4] = TermColor::FromRgb(0x00, 0x37, 0xDA);
    Ansi16[5] = TermColor::FromRgb(0x88, 0x17, 0x98);
    Ansi16[6] = TermColor::FromRgb(0x3A, 0x96, 0xDD);
    Ansi16[7] = TermColor::FromRgb(0xCC, 0xCC, 0xCC);
    Ansi16[8] = TermColor::FromRgb(0x76, 0x76, 0x76);
    Ansi16[9] = TermColor::FromRgb(0xE7, 0x48, 0x56);
    Ansi16[10] = TermColor::FromRgb(0x16, 0xC6, 0x0C);
    Ansi16[11] = TermColor::FromRgb(0xF9, 0xF1, 0xA5);
    Ansi16[12] = TermColor::FromRgb(0x3B, 0x78, 0xFF);
    Ansi16[13] = TermColor::FromRgb(0xB4, 0x00, 0x9E);
    Ansi16[14] = TermColor::FromRgb(0x61, 0xD6, 0xD6);
    Ansi16[15] = TermColor::FromRgb(0xF2, 0xF2, 0xF2);
}

TerminalTheme TerminalTheme::Dark() {
    return TerminalTheme();
}

TerminalTheme TerminalTheme::Light() {
    TerminalTheme theme;
    theme.Background = TermColor::FromRgb(0xF8, 0xF8, 0xF8);
    theme.Foreground = TermColor::FromRgb(0x1E, 0x1E, 0x1E);
    theme.Cursor = TermColor::FromRgb(0x1E, 0x1E, 0x1E);
    theme.Selection = TermColor::FromArgb(0xA0, 0xAD, 0xD6, 0xFF);
    theme.Ansi16[0] = TermColor::FromRgb(0x00, 0x00, 0x00);
    theme.Ansi16[1] = TermColor::FromRgb(0xCD, 0x31, 0x31);
    theme.Ansi16[2] = TermColor::FromRgb(0x0E, 0xBC, 0x0E);
    theme.Ansi16[3] = TermColor::FromRgb(0xCD, 0xCD, 0x00);
    theme.Ansi16[4] = TermColor::FromRgb(0x00, 0x00, 0xEE);
    theme.Ansi16[5] = TermColor::FromRgb(0xCD, 0x00, 0xCD);
    theme.Ansi16[6] = TermColor::FromRgb(0x00, 0xCD, 0xCD);
    theme.Ansi16[7] = TermColor::FromRgb(0xE5, 0xE5, 0xE5);
    theme.Ansi16[8] = TermColor::FromRgb(0x7F, 0x7F, 0x7F);
    theme.Ansi16[9] = TermColor::FromRgb(0xFF, 0x00, 0x00);
    theme.Ansi16[10] = TermColor::FromRgb(0x00, 0xFF, 0x00);
    theme.Ansi16[11] = TermColor::FromRgb(0xFF, 0xFF, 0x00);
    theme.Ansi16[12] = TermColor::FromRgb(0x5C, 0x5C, 0xFF);
    theme.Ansi16[13] = TermColor::FromRgb(0xFF, 0x00, 0xFF);
    theme.Ansi16[14] = TermColor::FromRgb(0x00, 0xFF, 0xFF);
    theme.Ansi16[15] = TermColor::FromRgb(0xFF, 0xFF, 0xFF);
    return theme;
}

} // namespace Term
} // namespace CUI
