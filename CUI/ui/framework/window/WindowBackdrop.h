#pragma once

#include <windows.h>
#include <dwmapi.h>

namespace CUI {

enum class BackdropType {
    None,
    Mica,
    MicaAlt,
    Acrylic
};

enum class ThemeMode {
    Dark,
    Light
};

class WindowBackdrop {
public:
    static bool ApplyBackdrop(HWND hwnd, BackdropType type);
    static bool ApplyTheme(HWND hwnd, ThemeMode theme);
};

} // namespace CUI
