#pragma once
#include "TerminalTheme.h"
#include <string>

namespace CUI {
namespace Term {

// Terminal geometry, font, and theme options.
struct TerminalOptions {
    // Preferred fonts for Powerline / Oh My Posh (Nerd Font first).
    // DirectWrite resolves a single family per format, so the renderer walks this
    // comma separated list and keeps the first family that is actually installed.
    static const char* DefaultFontFamily() {
        return "CaskaydiaCove Nerd Font Mono, Cascadia Mono NF, Cascadia Code NF, "
               "MesloLGS NF, Cascadia Mono, Consolas, Courier New";
    }

    int Cols = 80;
    int Rows = 24;
    int Scrollback = 1000;
    std::string FontFamily = DefaultFontFamily();
    float FontSize = 14.0f;
    float MinFontSize = 8.0f;
    float MaxFontSize = 48.0f;
    bool CursorBlink = true;
    bool ConvertEol = false;
    bool CopyOnSelect = false;
    bool ScrollOnOutput = true;
    TerminalTheme Theme = TerminalTheme::Dark();
    std::string WindowsMode;
};

} // namespace Term
} // namespace CUI
