#include "WindowBackdrop.h"
#include "../style/ThemeManager.h"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

namespace CUI {
namespace {
constexpr DWORD kDwmBackdropNone = 1;
constexpr DWORD kDwmBackdropMainWindow = 2;
constexpr DWORD kDwmBackdropTransientWindow = 3;
constexpr DWORD kDwmBackdropTabbedWindow = 4;

DWORD DwmBackdropValue(BackdropType type) {
    switch (type) {
    case BackdropType::Auto:
    case BackdropType::Mica:
        return kDwmBackdropMainWindow;
    case BackdropType::MicaAlt:
        return kDwmBackdropTabbedWindow;
    case BackdropType::Acrylic:
    case BackdropType::Blur:
        return kDwmBackdropTransientWindow;
    case BackdropType::None:
    case BackdropType::Solid:
    default:
        return kDwmBackdropNone;
    }
}
}

const char* MaterialHost::DisplayNameZh(BackdropType type) {
    switch (type) {
    case BackdropType::Auto: return "自动材质";
    case BackdropType::Solid: return "纯色";
    case BackdropType::Mica: return "云母";
    case BackdropType::MicaAlt: return "云母 Alt";
    case BackdropType::Acrylic: return "亚克力";
    case BackdropType::Blur: return "兼容模糊";
    case BackdropType::None:
    default:
        return "关闭";
    }
}

BackdropType MaterialHost::Cycle(BackdropType type) {
    switch (type) {
    case BackdropType::None: return BackdropType::Solid;
    case BackdropType::Solid: return BackdropType::Mica;
    case BackdropType::Mica: return BackdropType::MicaAlt;
    case BackdropType::MicaAlt: return BackdropType::Acrylic;
    case BackdropType::Acrylic: return BackdropType::Blur;
    case BackdropType::Blur:
    case BackdropType::Auto:
    default:
        return BackdropType::None;
    }
}

bool MaterialHost::Apply(HWND hwnd, BackdropType type, ThemeMode theme) {
    if (!hwnd) {
        ThemeManager::Instance().SetBackdropType(type);
        return false;
    }

    const bool themeApplied = ApplyTheme(hwnd, theme);
    bool backdropApplied = ApplyBackdrop(hwnd, type);
    BackdropType effective = type;
    if (!backdropApplied && type != BackdropType::None && type != BackdropType::Solid) {
        effective = BackdropType::Solid;
        backdropApplied = ApplyBackdrop(hwnd, effective);
    }
    ThemeManager::Instance().SetBackdropType(effective);
    return themeApplied && backdropApplied;
}

bool MaterialHost::ApplyBackdrop(HWND hwnd, BackdropType type) {
    if (!hwnd) return false;
    const DWORD value = DwmBackdropValue(type);
    const HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &value, sizeof(value));
    return SUCCEEDED(hr);
}

bool MaterialHost::ApplyTheme(HWND hwnd, ThemeMode theme) {
    if (!hwnd) return false;
    BOOL darkMode = (theme == ThemeMode::Dark) ? TRUE : FALSE;
    const HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    return SUCCEEDED(hr);
}

} // namespace CUI
