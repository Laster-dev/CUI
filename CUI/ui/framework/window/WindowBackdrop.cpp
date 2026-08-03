#include "WindowBackdrop.h"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

namespace CUI {

enum DWM_SYSTEMBACKDROP_TYPE {
    DWMSBT_AUTO = 0,
    DWMSBT_NONE = 1,
    DWMSBT_MAINWINDOW = 2,      // Mica
    DWMSBT_TRANSIENTWINDOW = 3, // Acrylic
    DWMSBT_TABBEDWINDOW = 4     // MicaAlt
};

bool WindowBackdrop::ApplyBackdrop(HWND hwnd, BackdropType type) {
    if (!hwnd) return false;

    DWORD backdropValue = DWMSBT_NONE;
    switch (type) {
    case BackdropType::Mica:
        backdropValue = DWMSBT_MAINWINDOW;
        break;
    case BackdropType::MicaAlt:
        backdropValue = DWMSBT_TABBEDWINDOW;
        break;
    case BackdropType::Acrylic:
        backdropValue = DWMSBT_TRANSIENTWINDOW;
        break;
    case BackdropType::None:
    default:
        backdropValue = DWMSBT_NONE;
        break;
    }

    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropValue, sizeof(backdropValue));
    return SUCCEEDED(hr);
}

bool WindowBackdrop::ApplyTheme(HWND hwnd, ThemeMode theme) {
    if (!hwnd) return false;

    BOOL darkMode = (theme == ThemeMode::Dark) ? TRUE : FALSE;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    return SUCCEEDED(hr);
}

} // namespace CUI
