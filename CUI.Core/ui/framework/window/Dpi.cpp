#include "Dpi.h"
#include <shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")

namespace CUI {

void EnsureProcessDpiAwareness() {
    static bool s_initialized = false;
    if (s_initialized) {
        return;
    }
    s_initialized = true;

    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware();
    }
}

float GetDpiScaleForWindow(HWND hwnd) {
    UINT dpi = 96;
    if (hwnd) {
        dpi = GetDpiForWindow(hwnd);
    }
    if (dpi == 0) {
        dpi = 96;
    }
    return static_cast<float>(dpi) / 96.0f;
}

float PhysicalToLogical(float value, HWND hwnd) {
    const float scale = GetDpiScaleForWindow(hwnd);
    if (scale <= 0.001f) {
        return value;
    }
    return value / scale;
}

float LogicalToPhysical(float value, HWND hwnd) {
    return value * GetDpiScaleForWindow(hwnd);
}

void ClientPhysicalToLogical(HWND hwnd, int physicalX, int physicalY, float& logicalX, float& logicalY) {
    const float scale = GetDpiScaleForWindow(hwnd);
    const float safeScale = (scale > 0.001f) ? scale : 1.0f;
    logicalX = static_cast<float>(physicalX) / safeScale;
    logicalY = static_cast<float>(physicalY) / safeScale;
}

bool TryGetCursorClientLogical(HWND hwnd, float& logicalX, float& logicalY) {
    if (!hwnd) {
        return false;
    }
    POINT pt{};
    if (!GetCursorPos(&pt)) {
        return false;
    }
    if (!ScreenToClient(hwnd, &pt)) {
        return false;
    }
    ClientPhysicalToLogical(hwnd, pt.x, pt.y, logicalX, logicalY);
    return true;
}

} // namespace CUI
