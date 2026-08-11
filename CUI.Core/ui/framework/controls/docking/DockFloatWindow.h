#pragma once

#include "../../render/GraphicsContext.h"
#include "../../core/Value.h"
#include <functional>
#include <memory>
#include <string>
#include <windows.h>

namespace CUI {

class DockManager;
class UIElement;

// Secondary HWND for a torn-off pane. Self-draws chrome; hosts one content element.
class DockFloatWindow {
public:
    DockFloatWindow();
    ~DockFloatWindow();

    DockFloatWindow(const DockFloatWindow&) = delete;
    DockFloatWindow& operator=(const DockFloatWindow&) = delete;

    bool Show(DockManager* manager,
              int paneIndex,
              HWND owner,
              Point screenDipTopLeft,
              Size clientDipSize);
    void Destroy();

    HWND GetHWND() const { return m_hwnd; }
    int GetPaneIndex() const { return m_paneIndex; }
    void RemapPaneIndexAfterClose(int closedIndex);

    using CloseCallback = std::function<void(DockFloatWindow*)>;
    void SetCloseCallback(CloseCallback cb) { m_onClose = std::move(cb); }

    static bool IsDockFloatHwnd(HWND hwnd);
    static DockFloatWindow* FromHwnd(HWND hwnd);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void EnsureClass();

    bool EnsureWindow(HWND owner);
    void Paint();
    void Relayout();
    void HandleMouse(UINT msg, int x, int y);
    Point ClientPhysicalToDip(int x, int y) const;
    void SetTitleFromPane();

    HWND m_hwnd = nullptr;
    HWND m_owner = nullptr;
    DockManager* m_manager = nullptr;
    int m_paneIndex = -1;
    std::shared_ptr<UIElement> m_content;
    GraphicsContext m_gfx;
    float m_dpiScale = 1.0f;
    bool m_deviceReady = false;
    UIElement* m_hovered = nullptr;
    UIElement* m_pressed = nullptr;
    CloseCallback m_onClose;
    std::wstring m_title = L"Tool Window";
    float m_headerH = 28.0f;
};

} // namespace CUI
