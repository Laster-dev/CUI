#pragma once

#include "../render/GraphicsContext.h"
#include "../core/Value.h"
#include <windows.h>

namespace CUI {

class ContextMenu;
class UIElement;

// Top-level WS_POPUP host so ContextMenu can paint outside the owner window (Qt-style).
class MenuPopupWindow {
public:
    MenuPopupWindow();
    ~MenuPopupWindow();

    MenuPopupWindow(const MenuPopupWindow&) = delete;
    MenuPopupWindow& operator=(const MenuPopupWindow&) = delete;

    bool Show(ContextMenu* menu, HWND owner, Point screenDipTopLeft, Size clientDipSize);
    void Hide();
    void Invalidate();

    HWND GetHWND() const { return m_hwnd; }
    ContextMenu* GetMenu() const { return m_menu; }
    bool IsVisible() const { return m_hwnd && IsWindowVisible(m_hwnd); }
    Point ClientDipToScreenDip(float clientX, float clientY) const;

    static bool IsMenuPopupHwnd(HWND hwnd);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void EnsureClass();

    bool EnsureWindow(HWND owner);
    void Paint();
    Point ClientPhysicalToDip(int x, int y) const;
    void HandleMouseMove(int x, int y);
    void HandleMouseButton(UINT msg, int x, int y);

    HWND m_hwnd = nullptr;
    HWND m_owner = nullptr;
    ContextMenu* m_menu = nullptr;
    GraphicsContext m_gfx;
    float m_dpiScale = 1.0f;
    UIElement* m_hovered = nullptr;
    UIElement* m_pressed = nullptr;
    bool m_deviceReady = false;
};

} // namespace CUI
