#pragma once

#include <windows.h>
#include <dwmapi.h>

namespace CUI {

enum class BackdropType {
    None,
    Auto,
    Solid,
    Mica,
    MicaAlt,
    Acrylic,
    Blur
};

enum class ThemeMode {
    Dark,
    Light
};

class MaterialHost {
public:
    static const char* DisplayNameZh(BackdropType type);
    static BackdropType Cycle(BackdropType type);
    static bool Apply(HWND hwnd, BackdropType type, ThemeMode theme);
    static bool ApplyBackdrop(HWND hwnd, BackdropType type);
    static bool ApplyTheme(HWND hwnd, ThemeMode theme);
};

// Legacy alias — prefer MaterialHost.
using WindowBackdrop = MaterialHost;

} // namespace CUI
