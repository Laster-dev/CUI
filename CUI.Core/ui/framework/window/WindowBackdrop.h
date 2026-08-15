#pragma once

#include <cstdint>
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

/**
 * @brief 窗口统一材质状态。
 *
 * 避免 Window / ThemeManager / GraphicsContext 各自维护互相不一致的隐式状态：
 * 一处计算、处处消费。页面缓存是否允许复用由代次（generation）校验决定。
 */
struct WindowMaterialState {
    BackdropType type = BackdropType::None;   // 目标背景材质类型
    bool dwmBackdropActive = false;           // DWM 材质是否已实际应用（含降级结果）
    bool requiresPerPixelAlpha = false;       // 交换链是否需要逐像素透明（Composition SwapChain）
    bool transparentSurface = false;          // 根表面是否透明（材质模式下清屏必须为全透明）
    bool sceneCacheAllowed = true;            // 页面场景缓存是否允许复用（实际由代次校验兜底）
    uint64_t generation = 0;                  // 材质代次：每次切换 +1，驱动场景缓存失效

    bool IsMaterialMode() const { return dwmBackdropActive; } // 是否处于 DWM 材质模式
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
