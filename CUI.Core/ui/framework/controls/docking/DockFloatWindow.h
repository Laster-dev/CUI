#pragma once

#include "../../render/GraphicsContext.h"
#include "../../core/Value.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace CUI {

class DockManager;
class UIElement;

// Torn-off pane hosted in a CUI-chrome ::HWND (custom title bar, no native caption).
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
              void* owner,
              Point screenDipTopLeft,
              Size windowDipSize);
    void Destroy();
    void DetachContent();

    void* GetHWND() const { return m_hwnd; }
    int GetPaneIndex() const { return m_paneIndex; }
    void RemapPaneIndexAfterClose(int closedIndex);

    using CloseCallback = std::function<void(DockFloatWindow*)>;
    void ApplyCloseCallback(CloseCallback cb) { m_onClose = std::move(cb); }

    static bool IsDockFloatHwnd(void* hwnd);
    static DockFloatWindow* FromHwnd(void* hwnd);
    // Called from the owner Window frame loop after AnimationManager::Tick.
    static void PresentAll();

private:
    static intptr_t __stdcall WndProc(void* hwnd, unsigned int msg, uintptr_t wParam, intptr_t lParam);
    static void EnsureClass();

    bool EnsureWindow(void* owner);
    void ApplyCuiChrome();
    void Paint();
    void Relayout();
    void HandleClientSize();
    void HandleMouse(unsigned int msg, int x, int y);
    void HandleWheel(float delta, int x, int y);
    void TrackClientMouse();
    Point ClientPhysicalToDip(int x, int y) const;
    void ApplyTitleFromPane();
    Rect TitleBarRect() const;
    Rect CaptionButtonRect(int index) const; // 0=min 1=max 2=close
    int HitCaptionButton(float x, float y) const;
    intptr_t HitTest(int screenX, int screenY) const;
    void ApplyCaptionHover(int region);
    Point CursorScreenDip() const;
    void BeginRedockTracking();
    void UpdateRedockTracking();
    void FinishRedockTracking();

    void* m_hwnd = nullptr;
    void* m_owner = nullptr;
    DockManager* m_manager = nullptr;
    int m_paneIndex = -1;
    std::shared_ptr<UIElement> m_content;
    GraphicsContext m_gfx;
    float m_dpiScale = 1.0f;
    bool m_deviceReady = false;
    unsigned int m_lastPxW = 0;
    unsigned int m_lastPxH = 0;
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
