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

// Unified material pipeline: DWM SystemBackdrop + ThemeManager paint alphas.
// Window / TitleBar / Gallery must go through this — never DIY DWM or opaque chrome fills.
class MaterialHost {
public:
    // 无材质 / 云母 / 沉浸云母 / 亚克力
    static const char* DisplayNameZh(BackdropType type);
    static BackdropType Cycle(BackdropType type);

    // ThemeManager alphas + DWM attributes + frame margins in one shot.
    static bool Apply(HWND hwnd, BackdropType type, ThemeMode theme);

    // Low-level DWM helpers (Apply calls these).
    static bool ApplyBackdrop(HWND hwnd, BackdropType type);
    static bool ApplyTheme(HWND hwnd, ThemeMode theme);
};

// Legacy alias — prefer MaterialHost.
using WindowBackdrop = MaterialHost;

} // namespace CUI
