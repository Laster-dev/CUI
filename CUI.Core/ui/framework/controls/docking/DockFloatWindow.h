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

// Torn-off pane hosted in a CUI-chrome HWND (custom title bar, no native caption).
// Content stays in the owner Window live tree so AnimationManager ticks it;
// this object only presents that already-ticked UI onto a second HWND.
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
              Size windowDipSize);
    void Destroy();
    void DetachContent();

    HWND GetHWND() const { return m_hwnd; }
    int GetPaneIndex() const { return m_paneIndex; }
    void RemapPaneIndexAfterClose(int closedIndex);

    using CloseCallback = std::function<void(DockFloatWindow*)>;
    void SetCloseCallback(CloseCallback cb) { m_onClose = std::move(cb); }

    static bool IsDockFloatHwnd(HWND hwnd);
    static DockFloatWindow* FromHwnd(HWND hwnd);
    // Called from the owner Window frame loop after AnimationManager::Tick.
    static void PresentAll();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void EnsureClass();

    bool EnsureWindow(HWND owner);
    void ApplyCuiChrome();
    void Paint();
    void Relayout();
    void HandleClientSize();
    void HandleMouse(UINT msg, int x, int y);
    void HandleWheel(float delta, int x, int y);
    void TrackClientMouse();
    Point ClientPhysicalToDip(int x, int y) const;
    void SetTitleFromPane();
    Rect TitleBarRect() const;
    Rect CaptionButtonRect(int index) const; // 0=min 1=max 2=close
    int HitCaptionButton(float x, float y) const;
    LRESULT HitTest(int screenX, int screenY) const;
    void SetCaptionHover(int region);
    Point CursorScreenDip() const;
    void BeginRedockTracking();
    void UpdateRedockTracking();
    void FinishRedockTracking();

    HWND m_hwnd = nullptr;
    HWND m_owner = nullptr;
    DockManager* m_manager = nullptr;
    int m_paneIndex = -1;
    std::shared_ptr<UIElement> m_content;
    GraphicsContext m_gfx;
    float m_dpiScale = 1.0f;
    bool m_deviceReady = false;
    UINT m_lastPxW = 0;
    UINT m_lastPxH = 0;
    UIElement* m_hovered = nullptr;
    UIElement* m_pressed = nullptr;
    CloseCallback m_onClose;
    std::wstring m_title = L"Tool Window";
    std::string m_titleUtf8 = "Tool Window";
    float m_titleH = 36.0f;
    int m_captionHover = -1;
    bool m_redocking = false;
    bool m_destroying = false;
    bool m_trackingMouse = false;
};

} // namespace CUI
