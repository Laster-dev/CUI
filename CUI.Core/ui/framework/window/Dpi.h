#pragma once
#include <windows.h>

namespace CUI {

// Per-monitor DPI helpers. Win32 client metrics are physical pixels; layout/D2D use DIPs.
void EnsureProcessDpiAwareness();

float GetDpiScaleForWindow(HWND hwnd);
float PhysicalToLogical(float value, HWND hwnd);
float LogicalToPhysical(float value, HWND hwnd);

// Convert client physical pixels (e.g. from ScreenToClient / mouse lParam) to DIPs.
void ClientPhysicalToLogical(HWND hwnd, int physicalX, int physicalY, float& logicalX, float& logicalY);

// GetCursorPos → ScreenToClient → DIPs. Returns false if hwnd/cursor unavailable.
bool TryGetCursorClientLogical(HWND hwnd, float& logicalX, float& logicalY);

} // namespace CUI
