#pragma once
#include "../controls/UIElement.h"
#include "../render/GraphicsContext.h"
#include "../render/CompositionContext.h"
#include "../render/DirtyRegion.h"
#include "../render/RenderLayer.h"
#include "../render/LayerRasterizer.h"
#include "../animation/AnimationManager.h"
#include "../animation/FrameScheduler.h"
#include "../input/RoutedEvent.h"
#include "WindowBackdrop.h"
#include "PopupHost.h"
#include <windows.h>
#include <chrono>
#include <memory>
#include <string>

namespace CUI {

class Window {
public:
    Window();
    virtual ~Window();

    // width/height are DIPs (design size); converted to physical pixels at Create time.
    // width/height are DIPs (logical design size); converted to physical for CreateWindowEx.
    bool Create(const std::string& title, int width = 1280, int height = 800, bool transparentMode = false);
    void Show();
    void RunMessageLoop();

    void SetRootElement(std::shared_ptr<UIElement> root);
    void Relayout();
    std::shared_ptr<UIElement> GetRootElement() const { return m_rootElement; }

    HWND GetHWND() const { return m_hwnd; }
    GraphicsContext& GetGraphicsContext() { return m_gfxContext; }
    CompositionContext& GetCompositionContext() { return m_compositionContext; }
    const CompositionContext& GetCompositionContext() const { return m_compositionContext; }
    PopupHost& GetPopupHost() { return m_popupHost; }

    bool IsTransparentMode() const { return m_transparentMode; }
    void SetTransparentMode(bool enabled);
    void SetLowPerformanceMode(bool enabled);
    bool IsLowPerformanceMode() const { return m_lowPerformanceMode; }

    void SetBackdropType(BackdropType type);
    BackdropType GetBackdropType() const { return m_backdropType; }

    void SetThemeMode(ThemeMode theme);
    ThemeMode GetThemeMode() const { return m_themeMode; }

    void SetActiveContextMenu(std::shared_ptr<ContextMenu> menu) { m_activeContextMenu = menu; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void OnPaint();
    void OnResize(UINT width, UINT height);
    void UpdateDwmChrome();
    bool OnMouseMove(int x, int y);
    bool OnLButtonDown(int x, int y);
    void OnLButtonDblClick(int x, int y);
    bool OnLButtonUp(int x, int y);
    void OnRButtonDown(int x, int y);
    // Caption / TitleBar band only — not popups or document content.
    UIElement* HitTestChrome(float x, float y) const;
    static std::shared_ptr<UIElement> CaptureElementRef(UIElement* element);
    std::shared_ptr<UIElement> LockElement(const std::weak_ptr<UIElement>& element) const;
    static bool NeedsContinuousMouseRedraw(UIElement* element);
    void SetHoveredElement(UIElement* element);
    void SetPressedElement(UIElement* element);
    void SetFocusedElement(UIElement* element);
    void InvalidateAnimatedRegions(bool animationStillActive);
    void CommitFrame(bool animationStillActive);
    void FlushLayoutIfNeeded();
    void DispatchRoutedPointer(RoutedEventType type, Point pt, UIElement* target);
    bool TryMoveFocus(bool forward);
    void RequestFullRepaint();
    void InvalidatePendingRenderRegions(bool fallbackToFullWindow);
    void DrawRenderStatsOverlay();
    void ApplyVisualState();
    Point ClientPointToLogical(int x, int y) const;

    HWND m_hwnd = nullptr;
    float m_dpiScale = 1.0f;
    Size m_logicalClientSize{ 0.0f, 0.0f };
    GraphicsContext m_gfxContext;
    std::shared_ptr<UIElement> m_rootElement;

    std::weak_ptr<UIElement> m_hoveredElement;
    std::weak_ptr<UIElement> m_pressedElement;
    std::weak_ptr<UIElement> m_focusedElement;
    std::shared_ptr<ContextMenu> m_activeContextMenu = nullptr;
    PopupHost m_popupHost;
    bool m_trackingMouse = false;
    bool m_transparentMode = false;
    Rect m_lastAnimationDirtyRect;
    bool m_hasLastAnimationDirtyRect = false;
    AnimationManager m_animationManager;
    FrameScheduler m_frameScheduler;
    CompositionContext m_compositionContext;
    LayerRasterizer m_layerRasterizer;
    DirtyRegion m_pendingDirtyRegion;
    RenderLayer m_sceneLayer;
    bool m_showRenderStatsOverlay = true;
    bool m_lowPerformanceMode = false;
    bool m_flushInputDirty = false;
    BackdropType m_backdropType = BackdropType::None;
    ThemeMode m_themeMode = ThemeMode::Dark;
    std::chrono::steady_clock::time_point m_overlayFpsSampleStart{};
    unsigned m_overlayFrameCounter = 0;
    float m_overlayFps = 0.0f;
};

} // namespace CUI
