#include "WindowBackdrop.h"
#include "../style/ThemeManager.h"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace CUI {

const char* MaterialHost::DisplayNameZh(BackdropType) {
    return "关闭";
}

BackdropType MaterialHost::Cycle(BackdropType) {
    return BackdropType::None;
}

bool MaterialHost::Apply(HWND hwnd, BackdropType, ThemeMode theme) {
    ThemeManager::Instance().SetBackdropType(BackdropType::None);
    return ApplyTheme(hwnd, theme);
}

bool MaterialHost::ApplyBackdrop(HWND, BackdropType) {
    return true;
}

bool MaterialHost::ApplyTheme(HWND hwnd, ThemeMode theme) {
    ThemeManager::Instance().SetBackdropType(BackdropType::None);
    if (!hwnd) {
        return false;
    }

    BOOL darkMode = (theme == ThemeMode::Dark) ? TRUE : FALSE;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    return SUCCEEDED(hr);
}

} // namespace CUI
