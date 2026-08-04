#include "WindowBackdrop.h"
#include "../style/ThemeManager.h"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

#ifndef DWMWA_REDIRECTIONBITMAP_ALPHA
#define DWMWA_REDIRECTIONBITMAP_ALPHA 39
#endif

#ifndef DWMWA_USE_HOSTBACKDROPBRUSH
#define DWMWA_USE_HOSTBACKDROPBRUSH 17
#endif

namespace CUI {

enum DWM_SYSTEMBACKDROP_TYPE {
    DWMSBT_AUTO = 0,
    DWMSBT_NONE = 1,
    DWMSBT_MAINWINDOW = 2,      // 云母
    DWMSBT_TRANSIENTWINDOW = 3, // 亚克力
    DWMSBT_TABBEDWINDOW = 4     // 沉浸云母
};

namespace {

enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_ENABLE_HOSTBACKDROP = 5
};

struct ACCENT_POLICY {
    ACCENT_STATE AccentState;
    int AccentFlags;
    DWORD GradientColor; // AABBGGRR
    int AnimationId;
};

enum WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

using PFN_SetWindowCompositionAttribute =
    BOOL(WINAPI*)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

void ApplyAccentPolicy(HWND hwnd, BackdropType type, bool light) {
    static PFN_SetWindowCompositionAttribute pSet = nullptr;
    static bool resolved = false;
    if (!resolved) {
        resolved = true;
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32) {
            pSet = reinterpret_cast<PFN_SetWindowCompositionAttribute>(
                GetProcAddress(user32, "SetWindowCompositionAttribute"));
        }
    }
    if (!pSet) return;

    ACCENT_POLICY policy = {};
    switch (type) {
    case BackdropType::Acrylic:
        // 强磨砂：与「无材质」拉开可见差距（Composition 路径下作补充）
        policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
        policy.AccentFlags = 2 | 0x20 | 0x40 | 0x80 | 0x100;
        policy.GradientColor = light ? 0x66F3F3F3u : 0x991E1E1Eu;
        break;
    case BackdropType::Mica:
    case BackdropType::MicaAlt:
        policy.AccentState = ACCENT_ENABLE_HOSTBACKDROP;
        policy.AccentFlags = 0;
        policy.GradientColor = 0;
        break;
    case BackdropType::None:
    default:
        policy.AccentState = ACCENT_DISABLED;
        break;
    }

    WINDOWCOMPOSITIONATTRIBDATA data = {};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &policy;
    data.cbData = sizeof(policy);
    pSet(hwnd, &data);
}

void ApplyAlphaCompositing(HWND hwnd, bool enable) {
    BOOL useAlpha = enable ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_REDIRECTIONBITMAP_ALPHA, &useAlpha, sizeof(useAlpha));

    BOOL hostBackdrop = enable ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_HOSTBACKDROPBRUSH, &hostBackdrop, sizeof(hostBackdrop));

    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = enable ? TRUE : FALSE;
    bb.hRgnBlur = enable ? CreateRectRgn(0, 0, -1, -1) : nullptr;
    DwmEnableBlurBehindWindow(hwnd, &bb);
    if (bb.hRgnBlur) {
        DeleteObject(bb.hRgnBlur);
    }
}

void ApplyFrameMargins(HWND hwnd, bool enableBackdrop) {
    const bool maximized = hwnd && IsZoomed(hwnd) != FALSE;
    const MARGINS margins = enableBackdrop
        ? MARGINS{ -1, -1, -1, -1 }
        : (maximized ? MARGINS{ 0, 0, 0, 0 } : MARGINS{ 1, 1, 1, 1 });
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

} // namespace

const char* MaterialHost::DisplayNameZh(BackdropType type) {
    switch (type) {
    case BackdropType::Mica: return "云母";
    case BackdropType::MicaAlt: return "沉浸云母";
    case BackdropType::Acrylic: return "亚克力";
    case BackdropType::None:
    default: return "无材质";
    }
}

BackdropType MaterialHost::Cycle(BackdropType type) {
    switch (type) {
    case BackdropType::Mica: return BackdropType::MicaAlt;
    case BackdropType::MicaAlt: return BackdropType::Acrylic;
    case BackdropType::Acrylic: return BackdropType::None;
    case BackdropType::None:
    default: return BackdropType::Mica;
    }
}

bool MaterialHost::Apply(HWND hwnd, BackdropType type, ThemeMode theme) {
    ThemeManager::Instance().SetBackdropType(type);
    if (!hwnd) {
        return false;
    }
    ApplyTheme(hwnd, theme);
    const bool ok = ApplyBackdrop(hwnd, type);
    ApplyFrameMargins(hwnd, type != BackdropType::None);
    return ok;
}

bool MaterialHost::ApplyBackdrop(HWND hwnd, BackdropType type) {
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

    const bool enableAlpha = (type != BackdropType::None);
    const bool light = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;
    ApplyAlphaCompositing(hwnd, enableAlpha);
    ApplyAccentPolicy(hwnd, type, light);
    ApplyFrameMargins(hwnd, enableAlpha);

    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropValue, sizeof(backdropValue));
    return SUCCEEDED(hr);
}

bool MaterialHost::ApplyTheme(HWND hwnd, ThemeMode theme) {
    if (!hwnd) return false;

    BOOL darkMode = (theme == ThemeMode::Dark) ? TRUE : FALSE;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    return SUCCEEDED(hr);
}

} // namespace CUI
