#include "Window.h"
#include "Dpi.h"
#include "../style/ThemeManager.h"
#include "../parser/StyleManager.h"
#include "../controls/TextBox.h"
#include "../controls/ContextMenu.h"
#include "../controls/VSCodeControls.h"
#include "../controls/DatePicker.h"
#include "../controls/TimePicker.h"
#include "../controls/ColorPicker.h"
#include "../controls/ComboBox.h"
#include "../controls/Flyout.h"
#include "../animation/FrameScheduler.h"
#include "../input/RoutedEvent.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <imm.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <sstream>

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

namespace CUI {

namespace {
constexpr UINT WM_CUI_TOGGLE_LOW_PERF = WM_APP + 42;
constexpr UINT WM_CUI_TOGGLE_BACKDROP = WM_APP + 43;
constexpr UINT WM_CUI_TOGGLE_THEME = WM_APP + 44;
constexpr UINT WM_CUI_RASTER_COMPLETE = WM_APP + 45;

float GetWindowRefreshRateHz(HWND hwnd) {
    // EnumDisplaySettings is relatively expensive — cache per monitor briefly.
    struct Cache {
        HMONITOR monitor = nullptr;
        float hz = 60.0f;
        std::chrono::steady_clock::time_point at{};
    };
    static Cache cache;

    if (!hwnd) {
        return 60.0f;
    }

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    const auto now = std::chrono::steady_clock::now();
    if (cache.monitor == monitor
        && std::chrono::duration<float>(now - cache.at).count() < 2.0f
        && cache.hz > 1.0f) {
        return cache.hz;
    }

    MONITORINFOEX monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfo(monitor, &monitorInfo)) {
        return 60.0f;
    }

    DEVMODE devMode = {};
    devMode.dmSize = sizeof(devMode);
    if (!EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode)) {
        return 60.0f;
    }

    const DWORD hz = devMode.dmDisplayFrequency;
    cache.monitor = monitor;
    cache.at = now;
    cache.hz = (hz == 0 || hz == 1) ? 60.0f : static_cast<float>(hz);
    return cache.hz;
}

bool CoversRect(const Rect& outer, const Rect& inner, float epsilon = 1.0f) {
    if (inner.IsEmpty()) {
        return true;
    }
    if (outer.IsEmpty()) {
        return false;
    }

    return outer.x <= inner.x + epsilon
        && outer.y <= inner.y + epsilon
        && outer.x + outer.width >= inner.x + inner.width - epsilon
        && outer.y + outer.height >= inner.y + inner.height - epsilon;
}

// True only if some individual dirty rect covers the viewport — NOT the AABB of
// disjoint rects (pane ripple + content dirty), which would false-trigger full repaints.
bool AnyDirtyRectCovers(const DirtyRegion& region, const Rect& viewport, float epsilon = 1.0f) {
    for (const auto& rect : region.GetRects()) {
        if (CoversRect(rect, viewport, epsilon)) {
            return true;
        }
    }
    return false;
}

bool IsOverlayScrimAnimating(UIElement* element) {
    if (!element) {
        return false;
    }
    // Only freeze the scene during ContentDialog open/close animation.
    // Freezing for the whole modal lifetime left under-scrim button ripples stuck
    // (ticks advanced, but dirty rects were discarded without updating the cache).
    if (element->HasSelfAnimation() && element->IsModalOverlayOpen()) {
        return true;
    }
    for (const auto& child : element->GetChildren()) {
        if (IsOverlayScrimAnimating(child.get())) {
            return true;
        }
    }
    return false;
}

Rect GetClientBounds(HWND hwnd) {
    RECT rc = {};
    if (hwnd) {
        GetClientRect(hwnd, &rc);
    }
    return Rect(
        static_cast<float>(rc.left),
        static_cast<float>(rc.top),
        static_cast<float>(rc.right - rc.left),
        static_cast<float>(rc.bottom - rc.top)
    );
}

Rect PhysicalRectToLogical(const Rect& rect, float dpiScale) {
    const float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return Rect(
        rect.x / scale,
        rect.y / scale,
        rect.width / scale,
        rect.height / scale
    );
}

Rect GetLogicalClientBounds(HWND hwnd, float dpiScale) {
    return PhysicalRectToLogical(GetClientBounds(hwnd), dpiScale);
}

// Paint always resolves colors through ResolveThemeColor(Get*Token()), so this
// walk only needs to seed sensible default tokens where a control hasn't set
// its own — no legacy ColorF/string mirroring required.
void ApplyThemeToTree(UIElement* element, bool systemBackdrop) {
    if (!element) {
        return;
    }

    const std::string className = element->GetClassName();
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    if (className == "TitleBar") {
        // Keep chrome fill stable across hover/focus; always sync from titleBarBackground.
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::TitleBarBackground);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackgroundToken(ThemeTokenId::TitleBarBackground);
        }
        if (element->GetPressedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetPressedBackgroundToken(ThemeTokenId::TitleBarBackground);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TitleBarText);
        }
        element->SetBackground(ThemeManager::Instance().GetColor("titleBarBackground"));
        element->SetHoverBackground(ThemeManager::Instance().GetColor("titleBarBackground"));
        element->SetPressedBackground(ThemeManager::Instance().GetColor("titleBarBackground"));
        element->SetColor(ThemeManager::Instance().GetColor("titleBarText"));
    } else if (className == "MenuBar") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::TitleBarBackground);
            element->SetBackground(ThemeManager::Instance().GetColor("titleBarBackground"));
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.titleBarText);
        }
    } else if (className == "Button") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.accentForeground);
        }
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.accentColor);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackground(tokens.accentColor);
        }
        if (element->GetPressedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetPressedBackground(tokens.accentColor);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderBrush(tokens.accentColor);
        }
    } else if (className == "PropertyGrid") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::PaneBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
    } else if (className == "ContextMenu") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.cardBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderBrush(tokens.cardBorder);
        }
    } else if (className == "MenuItem") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textSecondary);
        }
    } else if (className == "TextBox") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textPrimary);
        }
    } else if (className == "ComboBox") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::InputBackground);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::InputBorder);
        }
        if (element->GetFocusedBorderToken() == ThemeTokenId::Unset) {
            element->SetFocusedBorderToken(ThemeTokenId::FocusedBorder);
        }
        if (element->GetDropdownBackgroundToken() == ThemeTokenId::Unset) {
            element->SetDropdownBackgroundToken(ThemeTokenId::CardBackground);
        }
        if (element->GetSelectedItemBackgroundToken() == ThemeTokenId::Unset) {
            element->SetSelectedItemBackgroundToken(ThemeTokenId::SelectedBackground);
        }
    } else if (className == "CheckBox") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textPrimary);
        }
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.inputBackground);
        }
    } else if (className == "RadioButton") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textPrimary);
        }
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.inputBackground);
        }
    } else if (className == "ToggleSwitch") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textPrimary);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderBrush(tokens.cardBorder);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackground(tokens.hoverBackground);
        }
        if (element->GetPressedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetPressedBackground(tokens.pressedBackground);
        }
    } else if (className == "Slider") {
        if (element->GetTrackColorToken() == ThemeTokenId::Unset) {
            element->SetTrackColorToken(ThemeTokenId::CardBorder);
        }
        if (element->GetActiveTrackColorToken() == ThemeTokenId::Unset) {
            element->SetActiveTrackColorToken(ThemeTokenId::AccentColor);
        }
        if (element->GetThumbColorToken() == ThemeTokenId::Unset) {
            element->SetThumbColorToken(ThemeTokenId::AccentColor);
        }
    } else if (className == "CollapsePanel") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::PaneBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
    } else if (className == "ListBox") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::CardBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
        if (element->GetSelectedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        }
    } else if (className == "ListView") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::CardBackground);
        }
        if (element->GetHeaderBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHeaderBackgroundToken(ThemeTokenId::PaneBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
        if (element->GetGridLineBrushToken() == ThemeTokenId::Unset) {
            element->SetGridLineBrushToken(ThemeTokenId::InputBorder);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
        if (element->GetSelectedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        }
    } else if (className == "TabView") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.windowBackground);
        }
    } else if (className == "TreeView") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::CardBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderToken(ThemeTokenId::CardBorder);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
        if (element->GetSelectedBackgroundToken() == ThemeTokenId::Unset) {
            element->SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
        }
        if (element->GetHoverBackgroundToken() == ThemeTokenId::Unset) {
            element->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        }
    } else if (className == "BreadcrumbBar") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.cardBackground);
        }
        if (element->GetBorderToken() == ThemeTokenId::Unset) {
            element->SetBorderBrush(tokens.cardBorder);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.textSecondary);
        }
    } else if (className == "NavigationView") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.windowBackground);
        }
    } else if (className == "NavigationViewItem" || className == "NavigationViewItemHeader") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColorToken(ThemeTokenId::TextPrimary);
        }
    } else if (className == "TextBlock" || className == "HyperlinkButton") {
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(className == "HyperlinkButton" ? tokens.accentColor : tokens.textSecondary);
        }
    } else if (className == "ActivityBar") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::ActivityBarBackground);
            element->SetBackground(ThemeManager::Instance().GetColor("activityBarBackground"));
        }
    } else if (className == "SideBar" || className == "TabBar") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::PaneBackground);
            element->SetBackground(ThemeManager::Instance().GetColor("paneBackground"));
        }
    } else if (className == "EditorView") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackgroundToken(ThemeTokenId::WindowBackground);
            element->SetBackground(ThemeManager::Instance().GetColor("windowBackground"));
        }
    } else if (className == "StatusBar") {
        if (element->GetBackgroundToken() == ThemeTokenId::Unset) {
            element->SetBackground(tokens.accentColor);
        }
        if (element->GetColorToken() == ThemeTokenId::Unset) {
            element->SetColor(tokens.accentForeground);
        }
    }

    for (const auto& child : element->GetChildren()) {
        ApplyThemeToTree(child.get(), systemBackdrop);
    }
}

void ForceThemeRefresh(UIElement* element, const std::string& refreshStamp, bool systemBackdrop) {
    if (!element) {
        return;
    }

    // Force render cache invalidation across the tree instead of mirroring a
    // nonce through the string property bridge (nothing reads that string).
    element->MarkRenderContentDirty();

    if (auto* menu = dynamic_cast<ContextMenu*>(element)) {
        if (auto subMenu = menu->GetActiveSubMenu()) {
            ApplyThemeToTree(subMenu.get(), systemBackdrop);
            ForceThemeRefresh(subMenu.get(), refreshStamp, systemBackdrop);
        }
    }

    for (const auto& child : element->GetChildren()) {
        ForceThemeRefresh(child.get(), refreshStamp, systemBackdrop);
    }
}
}

Window::Window() {
    m_sceneLayer.SetCacheable(true);
}

std::shared_ptr<UIElement> Window::CaptureElementRef(UIElement* element) {
    if (!element) return nullptr;
    try {
        return std::dynamic_pointer_cast<UIElement>(element->shared_from_this());
    } catch (const std::bad_weak_ptr&) {
        return nullptr;
    }
}

std::shared_ptr<UIElement> Window::LockElement(const std::weak_ptr<UIElement>& element) const {
    return element.lock();
}

bool Window::NeedsContinuousMouseRedraw(UIElement* element) {
    (void)element;
    // TitleBar/MenuBar already mark local dirty when hover index changes.
    // Returning true here forced a full title-bar Present on every WM_MOUSEMOVE pixel.
    return false;
}

void Window::SetHoveredElement(UIElement* element) {
    m_hoveredElement = CaptureElementRef(element);
}

void Window::SetPressedElement(UIElement* element) {
    m_pressedElement = CaptureElementRef(element);
}

void Window::SetFocusedElement(UIElement* element) {
    m_focusedElement = CaptureElementRef(element);
}

void Window::RequestFullRepaint() {
    if (!m_hwnd) {
        return;
    }

    m_sceneLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty | RenderLayer::SizeDirty);
    m_pendingDirtyRegion.Clear();
    if (m_rootElement) {
        m_pendingDirtyRegion.AddRect(m_rootElement->GetBounds());
    }
    RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void Window::InvalidatePendingRenderRegions(bool fallbackToFullWindow) {
    if (!m_hwnd) {
        return;
    }

    if (m_rootElement) {
        m_rootElement->CollectRenderDirtyRegion(m_pendingDirtyRegion, true);
    }

    if (m_pendingDirtyRegion.IsEmpty()) {
        if (fallbackToFullWindow) {
            RequestFullRepaint();
        }
        return;
    }

    const auto& rects = m_pendingDirtyRegion.GetRects();
    if (rects.size() > 8) {
        RequestFullRepaint();
        return;
    }

    for (const auto& rect : rects) {
        if (rect.IsEmpty()) {
            continue;
        }

        RECT rc = {
            static_cast<LONG>(std::floor(rect.x)),
            static_cast<LONG>(std::floor(rect.y)),
            static_cast<LONG>(std::ceil(rect.x + rect.width)),
            static_cast<LONG>(std::ceil(rect.y + rect.height))
        };
        InvalidateRect(m_hwnd, &rc, FALSE);
    }
}

void Window::InvalidateAnimatedRegions(bool animationStillActive) {
    if (!m_hwnd || !m_rootElement) {
        return;
    }

    Rect dirtyRect;
    bool hasDirty = false;
    // Only registered animators — full-tree CollectAnimationBounds was O(all controls)
    // every hover/scroll frame.
    m_animationManager.CollectAnimatingBounds(dirtyRect, hasDirty);
    m_popupHost.CollectDirty(dirtyRect, hasDirty);

    if (m_hasLastAnimationDirtyRect) {
        dirtyRect = hasDirty ? dirtyRect.Union(m_lastAnimationDirtyRect) : m_lastAnimationDirtyRect;
        hasDirty = true;
    }

    if (hasDirty) {
        Rect expandedDirtyRect = dirtyRect.Inflate(2.0f);
        m_pendingDirtyRegion.AddRect(expandedDirtyRect);
        InvalidatePendingRenderRegions(false);

        if (animationStillActive) {
            m_lastAnimationDirtyRect = expandedDirtyRect;
            m_hasLastAnimationDirtyRect = true;
        } else {
            m_lastAnimationDirtyRect = Rect();
            m_hasLastAnimationDirtyRect = false;
        }
    } else if (!animationStillActive) {
        m_lastAnimationDirtyRect = Rect();
        m_hasLastAnimationDirtyRect = false;
    }

    // ContentDialog fade is overlay-only and does not contribute scene dirty.
    // Still force a full-client paint so the scrim/card composite updates.
    if (IsOverlayScrimAnimating(m_rootElement.get())) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void Window::CommitFrame(bool animationStillActive) {
    // Drain any UI-thread raster jobs; KickAsync covers worker-bound PictureLayer work.
    if (!m_layerRasterizer.KickAsync()) {
        m_layerRasterizer.FlushSync(m_gfxContext);
    }
    InvalidateAnimatedRegions(animationStillActive);
    InvalidatePendingRenderRegions(false);
}

void Window::FlushLayoutIfNeeded() {
    if (!m_rootElement || !m_hwnd) {
        return;
    }
    if (!m_rootElement->IsMeasureDirty() && !m_rootElement->IsArrangeDirty()) {
        return;
    }
    RECT rc{};
    GetClientRect(m_hwnd, &rc);
    const float padLeft = 0.0f;
    const float padTop = 0.0f;
    const float layoutW = m_logicalClientSize.width > 0.0f
        ? m_logicalClientSize.width
        : static_cast<float>(rc.right - rc.left) / (m_dpiScale > 0.0f ? m_dpiScale : 1.0f);
    const float layoutH = m_logicalClientSize.height > 0.0f
        ? m_logicalClientSize.height
        : static_cast<float>(rc.bottom - rc.top) / (m_dpiScale > 0.0f ? m_dpiScale : 1.0f);
    m_rootElement->FlushLayout(Size(layoutW, layoutH), Rect(padLeft, padTop, layoutW, layoutH));
}

void Window::DispatchRoutedPointer(RoutedEventType type, Point pt, UIElement* target) {
    if (!target) {
        return;
    }
    std::vector<UIElement*> path;
    for (UIElement* walk = target; walk; walk = walk->GetParent()) {
        path.push_back(walk);
    }

    RoutedEventArgs args;
    args.type = type;
    args.position = pt;
    args.originalSource = target;

    args.phase = RoutedEventPhase::Tunnel;
    for (auto it = path.rbegin(); it != path.rend() && !args.handled; ++it) {
        if (*it != target) {
            (*it)->OnRoutedEvent(args);
        }
    }

    if (!args.handled) {
        args.phase = RoutedEventPhase::Target;
        target->OnRoutedEvent(args);
    }

    args.phase = RoutedEventPhase::Bubble;
    for (UIElement* walk : path) {
        if (args.handled) {
            break;
        }
        if (walk != target) {
            walk->OnRoutedEvent(args);
        }
    }
}

bool Window::TryMoveFocus(bool forward) {
    if (!m_rootElement) {
        return false;
    }
    std::vector<UIElement*> focusable;
    std::function<void(UIElement*)> walk = [&](UIElement* el) {
        if (!el || el->GetVisibility() != Visibility::Visible || !el->IsEnabled()) {
            return;
        }
        // TextBox and similar accept focus via classic focus path.
        if (dynamic_cast<TextBox*>(el)) {
            focusable.push_back(el);
        }
        for (auto& child : el->GetChildren()) {
            walk(child.get());
        }
    };
    walk(m_rootElement.get());
    if (focusable.empty()) {
        return false;
    }

    int index = -1;
    if (auto focused = LockElement(m_focusedElement)) {
        for (int i = 0; i < static_cast<int>(focusable.size()); ++i) {
            if (focusable[i] == focused.get()) {
                index = i;
                break;
            }
        }
    }
    int next = forward
        ? (index + 1) % static_cast<int>(focusable.size())
        : (index <= 0 ? static_cast<int>(focusable.size()) - 1 : index - 1);
    SetFocusedElement(focusable[next]);
    return true;
}

Window::~Window() {
    if (FrameScheduler::Current() == &m_frameScheduler) {
        FrameScheduler::SetCurrent(nullptr);
    }
    if (AnimationManager::Current() == &m_animationManager) {
        AnimationManager::SetCurrent(nullptr);
    }
    if (PopupHost::Current() == &m_popupHost) {
        PopupHost::SetCurrent(nullptr);
    }
    m_popupHost.CloseAll();
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

bool Window::Create(const std::string& title, int width, int height, bool transparentMode) {
    EnsureProcessDpiAwareness();
    m_transparentMode = transparentMode;
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // BLACK_BRUSH zeros alpha on erase for any remaining GDI path.
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = L"CUI_WindowClass";

    RegisterClassEx(&wc);

    std::wstring wTitle(title.begin(), title.end());

    // DirectComposition host requires WS_EX_NOREDIRECTIONBITMAP so GDI's opaque
    // redirection surface does not cover premul swap-chain alpha.
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
#ifndef WS_EX_NOREDIRECTIONBITMAP
#define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif
    DWORD dwExStyle = WS_EX_NOREDIRECTIONBITMAP;
    if (m_transparentMode) {
        dwExStyle |= (WS_EX_LAYERED | WS_EX_APPWINDOW);
    }

    // Create() width/height are DIPs (design size). Win32 CreateWindowEx expects
    // physical outer size under Per-Monitor V2.
    UINT dpi = GetDpiForSystem();
    if (dpi == 0) {
        dpi = 96;
    }
    const int physWidth = MulDiv((std::max)(1, width), static_cast<int>(dpi), 96);
    const int physHeight = MulDiv((std::max)(1, height), static_cast<int>(dpi), 96);

    m_hwnd = CreateWindowEx(
        dwExStyle,
        L"CUI_WindowClass",
        wTitle.c_str(),
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        physWidth, physHeight,
        nullptr, nullptr,
        hInstance,
        this
    );

    if (!m_hwnd) return false;

    m_dpiScale = GetDpiScaleForWindow(m_hwnd);

    UpdateDwmChrome();
    MaterialHost::Apply(m_hwnd, m_backdropType, m_themeMode);

    if (m_transparentMode) {
        SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
    }

    if (!m_gfxContext.Initialize(m_hwnd)) {
        return false;
    }

    m_layerRasterizer.BindDevice(m_gfxContext.GetD2DDevice());
    m_layerRasterizer.SetCompletionCallback([hwnd = m_hwnd]() {
        if (hwnd) {
            PostMessage(hwnd, WM_CUI_RASTER_COMPLETE, 0, 0);
        }
    });
    // Worker ready; jobs still FlushSync unless callers enable async + KickAsync.
    m_layerRasterizer.SetAsyncEnabled(true);

    // Graphics may add WS_EX_NOREDIRECTIONBITMAP for the composition fallback —
    // re-apply DWM alpha/backdrop so the final present path is wired correctly.
    MaterialHost::Apply(m_hwnd, m_backdropType, m_themeMode);
    UpdateDwmChrome();

    PopupHost::SetCurrent(&m_popupHost);
    AnimationManager::SetCurrent(&m_animationManager);
    FrameScheduler::SetCurrent(&m_frameScheduler);
    return true;
}

void Window::SetBackdropType(BackdropType type) {
    m_backdropType = type;
    if (m_hwnd) {
        MaterialHost::Apply(m_hwnd, type, m_themeMode);
        UpdateDwmChrome();
        // 丢弃层缓存，避免上一种材质的不透明像素残留
        m_gfxContext.GetResources().ReleaseDeviceResources();
        m_sceneLayer.ResetCache();
        ApplyVisualState();
        RequestFullRepaint();
    } else {
        ThemeManager::Instance().SetBackdropType(type);
    }
}

void Window::SetThemeMode(ThemeMode theme) {
    m_themeMode = theme;
    ThemeManager::Instance().SetThemeMode(theme);
    StyleManager::Instance().ReloadFromTheme();
    if (m_hwnd) {
        MaterialHost::Apply(m_hwnd, m_backdropType, theme);
        // Drop cached brushes/layers so light/dark RGB cannot linger across themes.
        m_gfxContext.GetResources().ReleaseDeviceResources();
        m_sceneLayer.ResetCache();
        ApplyVisualState();
        RequestFullRepaint();
    }
}

void Window::SetTransparentMode(bool enabled) {
    m_transparentMode = enabled;
}

void Window::Show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        // The very first layout often happens before the shown window settles on
        // its final client metrics/DPI-backed swap-chain size. If we paint using
        // that pre-show geometry, text and 1px edges can land on fractional
        // device pixels and look soft until the first WM_SIZE arrives.
        // Force one post-show relayout so startup uses the same path as a manual
        // resize, which keeps the initial frame crisp.
        Relayout();
        RequestFullRepaint();
        UpdateWindow(m_hwnd);
    }
}

void Window::SetLowPerformanceMode(bool enabled) {
    if (m_lowPerformanceMode == enabled) {
        return;
    }

    m_lowPerformanceMode = enabled;
    UIElement::SetAnimationsEnabled(!enabled);

    if (m_rootElement) {
        UIElement::SetAnimationDeltaSeconds(1.0f / 60.0f);
        m_rootElement->OnAnimationTick();
        m_rootElement->SyncRenderState();
    }

    m_lastAnimationDirtyRect = Rect();
    m_hasLastAnimationDirtyRect = false;
    RequestFullRepaint();
}

void Window::RunMessageLoop() {
    MSG msg = {};
    using clock = std::chrono::steady_clock;
    bool animationActive = false;

    for (;;) {
        bool hadMessage = false;
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            hadMessage = true;
            if (msg.message == WM_QUIT) {
                return;
            }
            if (msg.message == WM_MOUSEMOVE) {
                MSG newest = msg;
                while (PeekMessage(&newest, m_hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE)) {
                    msg = newest;
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (m_flushInputDirty) {
            m_flushInputDirty = false;
            InvalidatePendingRenderRegions(false);
        }

        const bool wasAnimationActive = animationActive;
        const auto now = clock::now();
        const float refreshHz = GetWindowRefreshRateHz(m_hwnd);
        const double targetFps = m_lowPerformanceMode
            ? 8.0
            : (std::min)(60.0, static_cast<double>((std::max)(30.0f, refreshHz)));
        const auto targetFrame = std::chrono::duration_cast<clock::duration>(
            std::chrono::duration<double>(1.0 / targetFps));
        m_frameScheduler.SetMinFrameInterval(targetFrame);
        m_animationManager.SetTargetFrameSeconds(static_cast<float>(1.0 / targetFps));

        m_animationManager.DispatchDueWakes(now);
        if (m_animationManager.HasAnimating() || m_animationManager.ConsumeFrameRequest()) {
            m_frameScheduler.ScheduleFrame();
        }
        if (auto focused = LockElement(m_focusedElement)) {
            if (focused->NeedsAutoScrollTick()) {
                m_frameScheduler.ScheduleFrame();
            }
        }
        if (animationActive) {
            m_frameScheduler.ScheduleFrame();
        }

        const bool frameDue = m_frameScheduler.ConsumeDue(now);
        bool animating = animationActive;
        bool didFrame = false;
        if (frameDue && m_rootElement) {
            FlushLayoutIfNeeded();

            m_animationManager.BeginFrame(now, animationActive);
            UIElement::SetAnimationDeltaSeconds(m_animationManager.GetDeltaSeconds());
            animating = m_animationManager.Tick();
            if (m_popupHost.TickAnimations()) {
                animating = true;
            }
            if (auto focused = LockElement(m_focusedElement)) {
                if (focused->NeedsAutoScrollTick()) {
                    focused->OnAutoScrollTick();
                    animating = true;
                }
            }
            didFrame = true;
        }
        animationActive = animating;

        bool presented = false;
        if (didFrame) {
            CommitFrame(animating || wasAnimationActive);
            // Drain WM_PAINT now so Present(1,0) is the vsync clock. Sleeping
            // another minInterval on top of Present was capping us near ~30 FPS.
            MSG paintMsg = {};
            while (PeekMessage(&paintMsg, m_hwnd, WM_PAINT, WM_PAINT, PM_REMOVE)) {
                TranslateMessage(&paintMsg);
                DispatchMessage(&paintMsg);
                presented = true;
            }
            if (animating) {
                m_frameScheduler.ScheduleFrame();
            }
        }

        if (!hadMessage) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
                continue;
            }
            const bool wantContinuous = animationActive || m_animationManager.HasAnimating();
            if (wantContinuous) {
                // Present already blocked for vsync when we painted; do not sleep
                // another frame. If Commit produced no paint, yield briefly.
                const DWORD waitMs = (didFrame && presented) ? 0
                    : static_cast<DWORD>((std::max)(1, static_cast<int>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(targetFrame).count())));
                MsgWaitForMultipleObjectsEx(0, nullptr, waitMs, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            } else if (m_frameScheduler.HasPending() || m_animationManager.HasPendingWake()) {
                const int schedMs = m_frameScheduler.GetMsUntilDeadline(clock::now());
                const int wakeMs = m_animationManager.GetMsUntilNextWake(clock::now());
                int waitMs = -1;
                if (schedMs >= 0) {
                    waitMs = schedMs;
                }
                if (wakeMs >= 0) {
                    waitMs = (waitMs < 0) ? wakeMs : (std::min)(waitMs, wakeMs);
                }
                if (waitMs < 0) {
                    waitMs = 0;
                }
                MsgWaitForMultipleObjectsEx(
                    0, nullptr, static_cast<DWORD>(waitMs), QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            } else {
                WaitMessage();
            }
        }
    }
}

void Window::SetRootElement(std::shared_ptr<UIElement> root) {
    m_rootElement = root;
    m_animationManager.SetLiveRoot(m_rootElement.get());
    if (m_rootElement) {
        ApplyVisualState();
        m_rootElement->SyncRenderState();
    }
    Relayout();
}

void Window::ApplyVisualState() {
    const bool systemBackdrop = (m_backdropType != BackdropType::None);
    ThemeManager::Instance().SetBackdropType(m_backdropType);
    if (m_rootElement) {
        ApplyThemeToTree(m_rootElement.get(), systemBackdrop);
        static unsigned long long s_themeRefreshNonce = 0;
        const std::string refreshStamp = std::to_string(++s_themeRefreshNonce);
        ForceThemeRefresh(m_rootElement.get(), refreshStamp, systemBackdrop);
    }
    if (m_activeContextMenu) {
        ApplyThemeToTree(m_activeContextMenu.get(), systemBackdrop);
        static unsigned long long s_themeMenuRefreshNonce = 0;
        const std::string refreshStamp = std::to_string(++s_themeMenuRefreshNonce);
        ForceThemeRefresh(m_activeContextMenu.get(), refreshStamp, systemBackdrop);
    }
    if (m_hwnd) {
        UpdateDwmChrome();
        m_sceneLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty | RenderLayer::SizeDirty);
        RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
}

void Window::Relayout() {
    if (!m_hwnd) return;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    OnResize(
        static_cast<UINT>(std::max<LONG>(0, rc.right - rc.left)),
        static_cast<UINT>(std::max<LONG>(0, rc.bottom - rc.top))
    );
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<Window*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Win11 Snap Layout + custom title bar notes:
    // 1. Do not call DwmDefWindowProc for every non-client message up front.
    //    If DWM/DefWindowProc gets WM_NCCALCSIZE, WM_NCACTIVATE, or WM_NCPAINT
    //    before our custom handling, Windows can restore and paint the native
    //    caption/title bar over the CUI-drawn one.
    // 2. Keep WM_NCCALCSIZE returning 0 so the client area owns the whole window.
    //    That is what makes the CUI TitleBar the visible title bar.
    // 3. For Snap Layout hover, WM_NCHITTEST must return HTMAXBUTTON on the
    //    custom maximize button rectangle. That rectangle must be tested before
    //    the top resize border, otherwise a windowed app returns HTTOP/HTTOPRIGHT
    //    and Windows never sees a maximize button under the cursor.
    // 4. DwmDefWindowProc can still participate narrowly in non-client mouse
    //    movement, but it must not own the whole non-client message pipeline.
    switch (uMsg) {
    case WM_NCCALCSIZE:
        if (wParam == TRUE) {
            auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
            if (IsZoomed(m_hwnd)) {
                // Use the full maximized window rect as the client area so DWM does
                // not leave a 1px non-client strip (white border) around rcWork.
                params->rgrc[0] = params->rgrc[1];
            }
        }
        // Custom chrome must consume both NCCALCSIZE forms. During the hidden
        // CreateWindow path Windows can ask with wParam == FALSE; if that falls
        // through to DefWindowProc, the first client rect is the classic
        // OVERLAPPEDWINDOW area (for example 1280x780 becomes 1264x741). The
        // swap chain is then created too small, leaving an unpainted strip on
        // the right/bottom until the first manual resize.
        return 0;

    case WM_NCACTIVATE:
        return TRUE;

    case WM_NCPAINT:
        return 0;

    case WM_SYSCOMMAND:
        // Handle window system commands with native animations
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(m_hwnd, SW_MINIMIZE);
            return 0;
        } else if ((wParam & 0xFFF0) == SC_RESTORE) {
            ShowWindow(m_hwnd, SW_RESTORE);
            return 0;
        } else if ((wParam & 0xFFF0) == SC_MAXIMIZE) {
            ShowWindow(m_hwnd, SW_MAXIMIZE);
            return 0;
        }
        break;

    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        POINT clientPt = pt;
        ScreenToClient(m_hwnd, &clientPt);
        Point logicalPt = ClientPointToLogical(clientPt.x, clientPt.y);
        float fx = logicalPt.x;
        float fy = logicalPt.y;

        RECT rc;
        GetClientRect(m_hwnd, &rc);
        const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
        float winW = static_cast<float>(rc.right) / scale;
        float winH = static_cast<float>(rc.bottom) / scale;

        // TitleBar chrome toggles / menu must win over resize borders and caption drag.
        // Prefer direct TitleBar child lookup — tree HitTest can still be wrong if content leaks.
        if (fy >= 0 && fy <= 40.0f && fx < winW - 138.0f && m_rootElement) {
            TitleBar* titleBar = nullptr;
            for (const auto& child : m_rootElement->GetChildren()) {
                titleBar = dynamic_cast<TitleBar*>(child.get());
                if (titleBar) break;
            }
            if (!titleBar) {
                if (auto* hit = m_rootElement->HitTest(fx, fy)) {
                    titleBar = dynamic_cast<TitleBar*>(hit);
                }
            }
            if (titleBar) {
                if (titleBar->IsLowPerformanceToggleHit(fx, fy) ||
                    titleBar->IsBackdropToggleHit(fx, fy) ||
                    titleBar->IsThemeToggleHit(fx, fy) ||
                    titleBar->IsMenuBarHit(fx, fy)) {
                    return HTCLIENT;
                }
            }
        }

        // System buttons get first chance after chrome toggles (46 DIP each).
        constexpr float kCaptionBtnW = 46.0f;
        if (fy >= 0 && fy <= 40.0f) {
            if (fx >= winW - kCaptionBtnW) {
                return HTCLOSE;
            }
            if (fx >= winW - kCaptionBtnW * 2.0f && fx < winW - kCaptionBtnW) {
                return HTMAXBUTTON;
            }
            if (fx >= winW - kCaptionBtnW * 3.0f && fx < winW - kCaptionBtnW * 2.0f) {
                return HTMINBUTTON;
            }
        }

        // 2. Resizing border handles (8-direction border resize) - only when NOT maximized
        if (!IsZoomed(m_hwnd)) {
            const int borderThickness = (std::max)(1, static_cast<int>(std::lround(8.0f * scale)));
            bool left = (clientPt.x < borderThickness);
            bool right = (clientPt.x >= rc.right - borderThickness);
            bool top = (clientPt.y < borderThickness);
            bool bottom = (clientPt.y >= rc.bottom - borderThickness);

            if (top && left) return HTTOPLEFT;
            if (top && right) return HTTOPRIGHT;
            if (bottom && left) return HTBOTTOMLEFT;
            if (bottom && right) return HTBOTTOMRIGHT;
            if (left) return HTLEFT;
            if (right) return HTRIGHT;
            if (top) return HTTOP;
            if (bottom) return HTBOTTOM;
        }

        // 3. Custom TitleBar Hit-Testing & Windows 11 Snap Layouts Integration
        if (fy >= 0 && fy <= 40.0f) {
            // Drag window caption (only when clicking directly on empty TitleBar space, NOT child controls)
            if (fx < winW - 138.0f && m_rootElement) {
                UIElement* hit = m_rootElement->HitTest(fx, fy);
                if (auto titleBar = dynamic_cast<TitleBar*>(hit)) {
                    if (titleBar->IsLowPerformanceToggleHit(fx, fy) || titleBar->IsBackdropToggleHit(fx, fy) || titleBar->IsThemeToggleHit(fx, fy)) {
                        return HTCLIENT;
                    }
                    if (titleBar->IsMenuBarHit(fx, fy)) {
                        return HTCLIENT;
                    }
                }
                if (hit && std::string(hit->GetClassName()) != "TitleBar" && hit != m_rootElement.get()) {
                    return HTCLIENT; // All child controls inside titlebar (MenuBar, Buttons, etc.) process UI clicks!
                }
                return HTCAPTION;
            }
        }

        return HTCLIENT;
    }

    case WM_NCLBUTTONDOWN: {
        LRESULT res = DefWindowProc(m_hwnd, uMsg, wParam, lParam);
        if (wParam == HTCLOSE) {
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
            return 0;
        } else if (wParam == HTMAXBUTTON) {
            if (IsZoomed(m_hwnd)) {
                SendMessage(m_hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            } else {
                SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            }
            return 0;
        } else if (wParam == HTMINBUTTON) {
            SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            return 0;
        }
        return res;
    }

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_CUI_RASTER_COMPLETE:
        if (FrameScheduler* sched = FrameScheduler::Current()) {
            sched->ScheduleFrame();
        }
        InvalidatePendingRenderRegions(false);
        return 0;

    case WM_CUI_TOGGLE_LOW_PERF:
        SetLowPerformanceMode(!m_lowPerformanceMode);
        return 0;

    case WM_CUI_TOGGLE_BACKDROP:
        SetBackdropType(WindowBackdrop::Cycle(m_backdropType));
        return 0;

    case WM_CUI_TOGGLE_THEME:
        SetThemeMode(m_themeMode == ThemeMode::Dark ? ThemeMode::Light : ThemeMode::Dark);
        return 0;

    case WM_SIZE:
        UpdateDwmChrome();
        // Do not trust WM_SIZE's lParam in the custom-chrome startup path. A
        // queued/synchronous size message can still carry the old native
        // OVERLAPPEDWINDOW client size (for example height 741) even after
        // WM_NCCALCSIZE has made GetClientRect report the full 780px surface.
        // Always relayout from GetClientRect so stale WM_SIZE data cannot shrink
        // the root tree and leave an unpainted strip at the bottom.
        Relayout();
        RequestFullRepaint();
        return 0;

    case WM_DPICHANGED:
        {
            // When Windows changes the DPI for this HWND, it provides the
            // recommended outer window bounds. Apply them immediately and then
            // relayout/recreate the render target at the new DPI so text stays
            // crisp instead of being temporarily bitmap-scaled.
            const RECT* suggestedRect = reinterpret_cast<const RECT*>(lParam);
            if (suggestedRect) {
                SetWindowPos(
                    m_hwnd,
                    nullptr,
                    suggestedRect->left,
                    suggestedRect->top,
                    suggestedRect->right - suggestedRect->left,
                    suggestedRect->bottom - suggestedRect->top,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            }
            UpdateDwmChrome();
            Relayout();
            RequestFullRepaint();
        }
        return 0;

    case WM_MOUSEMOVE:
        {
            Rect oldMenuBounds;
            if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
                oldMenuBounds = m_activeContextMenu->GetTotalBounds();
            }

            bool moved = OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

            Rect newMenuBounds;
            if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
                newMenuBounds = m_activeContextMenu->GetTotalBounds();
            }

            bool menuBoundsChanged = (oldMenuBounds.x != newMenuBounds.x || oldMenuBounds.y != newMenuBounds.y ||
                                      oldMenuBounds.width != newMenuBounds.width || oldMenuBounds.height != newMenuBounds.height);

            if (moved || menuBoundsChanged) {
                if (!oldMenuBounds.IsEmpty()) {
                    m_pendingDirtyRegion.AddRect(oldMenuBounds.Inflate(4.0f));
                }
                if (!newMenuBounds.IsEmpty()) {
                    m_pendingDirtyRegion.AddRect(newMenuBounds.Inflate(4.0f));
                }
                // Defer flush to the message loop so a burst of hover changes coalesces
                // into one Collect+Invalidate instead of one Present setup per TextBox.
                m_flushInputDirty = true;
            }
            // No dirty → no Present (cursor-only moves stay free).
        }
        return 0;

    case WM_NCMOUSEMOVE:
        {
            // Narrow DWM handoff: enough for non-client hover behavior, without
            // letting DWM resurrect native caption painting via NCCALCSIZE/PAINT.
            LRESULT dwmResult = 0;
            DwmDefWindowProc(m_hwnd, uMsg, wParam, lParam, &dwmResult);
        }
        // Do NOT RequestFullRepaint — NC mouse move must not pump the scene.
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

    case WM_NCMOUSELEAVE:
        // Do NOT RequestFullRepaint on NC leave.
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

    case WM_LBUTTONDOWN:
        if (OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
            InvalidatePendingRenderRegions(true);
        }
        return 0;

    case WM_LBUTTONDBLCLK:
        OnLButtonDblClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidatePendingRenderRegions(true);
        return 0;

    case WM_LBUTTONUP:
        if (OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
            InvalidatePendingRenderRegions(true);
        }
        return 0;

    case WM_RBUTTONDOWN:
        OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidatePendingRenderRegions(true);
        return 0;

    case WM_MOUSEWHEEL: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(m_hwnd, &pt);
        Point logicalPt = ClientPointToLogical(pt.x, pt.y);
        float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;

        // Same hit order as mouse move/click: popup → overlay → tree.
        UIElement* target = m_popupHost.HitTest(logicalPt.x, logicalPt.y);
        if (!target && m_rootElement) {
            target = m_rootElement->HitTestOverlay(logicalPt.x, logicalPt.y);
            if (!target) {
                target = m_rootElement->HitTest(logicalPt.x, logicalPt.y);
            }
        }
        if (target) {
            target->OnMouseWheel(delta);
            InvalidatePendingRenderRegions(false);
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_F8) {
            m_showRenderStatsOverlay = !m_showRenderStatsOverlay;
            RequestFullRepaint();
            return 0;
        }
        if (wParam == VK_TAB) {
            const bool forward = (GetKeyState(VK_SHIFT) & 0x8000) == 0;
            if (TryMoveFocus(forward)) {
                InvalidatePendingRenderRegions(false);
                return 0;
            }
        }
        if (auto focused = LockElement(m_focusedElement)) {
            if (focused->IsEnabled()) {
                RoutedEventArgs args;
                args.type = RoutedEventType::KeyDown;
                args.phase = RoutedEventPhase::Target;
                args.keyCode = static_cast<int>(wParam);
                args.originalSource = focused.get();
                focused->OnRoutedEvent(args);
                if (!args.handled) {
                    // Bubble key to ancestors for global shortcuts.
                    args.phase = RoutedEventPhase::Bubble;
                    for (UIElement* walk = focused->GetParent(); walk && !args.handled; walk = walk->GetParent()) {
                        walk->OnRoutedEvent(args);
                    }
                }
                InvalidatePendingRenderRegions(true);
            }
        }
        return 0;

    case WM_CHAR:
        if (auto focused = LockElement(m_focusedElement)) {
            if (focused->IsEnabled()) {
                focused->OnCharInput(static_cast<wchar_t>(wParam));
                InvalidatePendingRenderRegions(true);
            }
        }
        return 0;

    case WM_IME_COMPOSITION:
        if (auto focused = LockElement(m_focusedElement)) {
            if (auto tb = dynamic_cast<TextBox*>(focused.get())) {
                HIMC hIMC = ImmGetContext(m_hwnd);
                if (hIMC) {
                    if (lParam & GCS_COMPSTR) {
                        LONG bytes = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, nullptr, 0);
                        if (bytes > 0) {
                            std::wstring compStr(bytes / sizeof(wchar_t), 0);
                            ImmGetCompositionStringW(hIMC, GCS_COMPSTR, &compStr[0], bytes);
                            tb->SetCompositionString(compStr);
                        } else {
                            tb->SetCompositionString(L"");
                        }
                    }
                    if (lParam & GCS_RESULTSTR) {
                        LONG bytes = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, nullptr, 0);
                        if (bytes > 0) {
                            std::wstring resultStr(bytes / sizeof(wchar_t), 0);
                            ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, &resultStr[0], bytes);
                            tb->CommitImeResult(resultStr);
                        } else {
                            tb->SetCompositionString(L"");
                        }
                    }
                    ImmReleaseContext(m_hwnd, hIMC);
                    InvalidatePendingRenderRegions(true);
                }
            }
        }
        break;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            auto hovered = LockElement(m_hoveredElement);
            HCURSOR hCur = hovered ? hovered->GetCursor() : nullptr;
            if (hCur) {
                SetCursor(hCur);
                return TRUE;
            }
        }
        break;

    case WM_MOUSELEAVE:
        if (auto hovered = LockElement(m_hoveredElement)) {
            hovered->OnMouseLeave();
            m_hoveredElement.reset();
            if (m_rootElement) {
                DirtyRegion probe;
                m_rootElement->CollectRenderDirtyRegion(probe, false);
                if (!probe.IsEmpty()) {
                    InvalidatePendingRenderRegions(false);
                }
            }
        }
        m_trackingMouse = false;
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void Window::OnPaint() {
    PAINTSTRUCT ps;
    BeginPaint(m_hwnd, &ps);

    const float dpiScale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
    Rect paintBounds = PhysicalRectToLogical(
        Rect(
            static_cast<float>(ps.rcPaint.left),
            static_cast<float>(ps.rcPaint.top),
            static_cast<float>(ps.rcPaint.right - ps.rcPaint.left),
            static_cast<float>(ps.rcPaint.bottom - ps.rcPaint.top)
        ),
        dpiScale
    );
    m_gfxContext.SetPaintBounds(paintBounds);
    DirtyRegion frameDirtyRegion = m_pendingDirtyRegion;
    if (frameDirtyRegion.IsEmpty()) {
        frameDirtyRegion.AddRect(paintBounds);
    }

    // Layout, hit-testing, and D2D drawing all use DIP coordinates.
    Rect viewportBounds(0.0f, 0.0f, m_logicalClientSize.width, m_logicalClientSize.height);
    if (viewportBounds.IsEmpty()) {
        viewportBounds = GetLogicalClientBounds(m_hwnd, dpiScale);
    }
    Rect dirtyBounds = frameDirtyRegion.GetBounds();
    const bool viewportDirty = frameDirtyRegion.GetRectCount() == 0
        || viewportBounds.IsEmpty()
        || CoversRect(paintBounds, viewportBounds)
        || AnyDirtyRectCovers(frameDirtyRegion, viewportBounds)
        || !m_sceneLayer.IsValid();
    // Always treat paint as full scene rebuild for the cache (avoids gap/margin black bars).
    const bool fullRepaint = true;
    (void)viewportDirty;
    (void)dirtyBounds;

    m_compositionContext.BeginFrame(viewportBounds, frameDirtyRegion, fullRepaint);
    m_gfxContext.SetCompositionContext(&m_compositionContext);

    m_gfxContext.BeginDraw();

    const bool systemBackdrop = (m_backdropType != BackdropType::None);
    const bool usePerPixelAlpha =
        systemBackdrop
        && m_gfxContext.SupportsPerPixelAlpha();
    // Transparent clear is required for DWM Mica/Acrylic to show through chrome.
    const D2D1_COLOR_F sceneClearColor = (usePerPixelAlpha || (m_transparentMode && !IsZoomed(m_hwnd)))
        ? D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)
        : ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground);

    const Size sceneSize(viewportBounds.width, viewportBounds.height);
    const bool sceneSizeChanged =
        std::abs(m_sceneLayer.GetCacheSurfaceSize().width - sceneSize.width) > 0.5f
        || std::abs(m_sceneLayer.GetCacheSurfaceSize().height - sceneSize.height) > 0.5f;

    auto renderScene = [&]() {
        // Scene layer must NOT include overlays — otherwise ContentDialog scrim is
        // baked into the cache and the reuse path redraws it again (flash/stutter).
        if (m_rootElement) {
            m_rootElement->Render(m_gfxContext);
        }
    };

    auto renderOverlaysOnly = [&]() {
        if (m_rootElement) {
            m_rootElement->RenderOverlay(m_gfxContext);
        }
        m_popupHost.Render(m_gfxContext);
    };

    // ContentDialog open/close: keep compositing overlay separately so the scrim
    // is never baked into the scene cache. Still patch under-scrim dirty rects
    // (button ripples, etc.) — skipping viewport-covering rects from the dialog
    // itself, which only need an overlay redraw.
    const bool overlayScrimAnimating = IsOverlayScrimAnimating(m_rootElement.get());
    const bool reuseSceneForOverlayAnim =
        overlayScrimAnimating
        && !sceneSizeChanged
        && m_sceneLayer.IsValid()
        && m_sceneLayer.GetCacheBitmap() != nullptr;

    if (reuseSceneForOverlayAnim) {
        // Reuse the last full scene bitmap; do not dirty-patch under the scrim
        // (same gap/margin seam risk as the main path).
        if (systemBackdrop || (m_transparentMode && !IsZoomed(m_hwnd))) {
            m_gfxContext.GetD2DContext()->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
        } else {
            m_gfxContext.GetD2DContext()->Clear(sceneClearColor);
        }
        m_gfxContext.DrawLayer(m_sceneLayer, viewportBounds);
        renderOverlaysOnly();
        DrawRenderStatsOverlay();
        m_gfxContext.EndDraw();
        m_gfxContext.SetCompositionContext(nullptr);
        m_compositionContext.EndFrame();
        m_pendingDirtyRegion.Clear();
        EndPaint(m_hwnd, &ps);
        return;
    }

    // Dirty-rect scene patching disabled: clearing StackPanel gaps / TextBlock margins
    // with WindowBackground or transparent left "black bars" on Pane/Card surfaces.
    // Rebuild the full scene bitmap each paint until gap-aware dirty expansion exists.
    if (m_gfxContext.PushLayerTarget(
            m_sceneLayer,
            sceneSize,
            viewportBounds,
            sceneClearColor,
            true)) {
        m_gfxContext.SetPaintBounds(viewportBounds);
        renderScene();
        m_gfxContext.PopLayerTarget(m_sceneLayer);
        m_sceneLayer.Validate();
    }

    if (systemBackdrop || (m_transparentMode && !IsZoomed(m_hwnd))) {
        m_gfxContext.GetD2DContext()->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    } else {
        m_gfxContext.GetD2DContext()->Clear(sceneClearColor);
    }
    m_gfxContext.DrawLayer(m_sceneLayer, viewportBounds);
    renderOverlaysOnly();
    DrawRenderStatsOverlay();

    m_gfxContext.EndDraw();
    m_gfxContext.SetCompositionContext(nullptr);
    m_compositionContext.EndFrame();
    m_pendingDirtyRegion.Clear();

    EndPaint(m_hwnd, &ps);
}

void Window::DrawRenderStatsOverlay() {
    if (!m_showRenderStatsOverlay) {
        return;
    }

    using clock = std::chrono::steady_clock;
    const auto now = clock::now();
    if (m_overlayFpsSampleStart.time_since_epoch().count() == 0) {
        m_overlayFpsSampleStart = now;
    }
    ++m_overlayFrameCounter;
    const auto elapsed = std::chrono::duration<float>(now - m_overlayFpsSampleStart).count();
    if (elapsed >= 0.5f) {
        m_overlayFps = m_overlayFrameCounter / elapsed;
        m_overlayFrameCounter = 0;
        m_overlayFpsSampleStart = now;
    }

    const auto& stats = m_compositionContext.GetStats();
    std::ostringstream ss;
    ss
        << "帧模式: " << (stats.fullRepaint ? "整帧" : "局部")
        << "  脏区: " << stats.dirtyRectCount
        << "  光栅: " << stats.rasterizedNodeCount
        << "  布局: " << stats.layoutPassCount
        << "  命中: " << stats.layerCacheHitCount
        << "  未命中: " << stats.layerCacheMissCount
        << "  重录: " << stats.layerCacheRerenderCount
        << "  复用: " << stats.layerCacheReuseCount
        << "  材质: " << MaterialHost::DisplayNameZh(m_backdropType)
        << (m_gfxContext.SupportsPerPixelAlpha() ? "" : "(无透)")
        << "  显示帧率: " << static_cast<int>(std::round(m_overlayFps));

    const std::string text = ss.str();
    Rect panel(12.0f, (std::max)(12.0f, m_rootElement ? (m_rootElement->GetBounds().height - 40.0f) : 12.0f), 820.0f, 28.0f);
    m_gfxContext.FillRoundedRect(panel, 6.0f, D2D1::ColorF(0.05f, 0.05f, 0.05f, 0.82f));
    m_gfxContext.DrawRoundedRect(panel, 6.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f), 1.0f);
    m_gfxContext.DrawText(
        text,
        Rect(panel.x + 10.0f, panel.y, panel.width - 20.0f, panel.height),
        D2D1::ColorF(0.92f, 0.92f, 0.92f, 1.0f),
        "Consolas",
        12.0f,
        DWRITE_TEXT_ALIGNMENT_LEADING,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER
    );
}

void Window::UpdateDwmChrome() {
    if (!m_hwnd) return;

    // System backdrop needs the full client area extended so translucent chrome
    // can reveal Mica/Acrylic. Otherwise keep a 1px DWM inset for snap animations.
    const bool maximized = IsZoomed(m_hwnd) != FALSE;
    const bool systemBackdrop = (m_backdropType != BackdropType::None);
    const MARGINS margins = systemBackdrop
        ? MARGINS{ -1, -1, -1, -1 }
        : (maximized ? MARGINS{ 0, 0, 0, 0 } : MARGINS{ 1, 1, 1, 1 });
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);

    // 1. Force Native Windows 11 DWM Rounded Corners (DWMWA_WINDOW_CORNER_PREFERENCE = 33)
    DWM_WINDOW_CORNER_PREFERENCE preference = maximized ? DWMWCP_DONOTROUND : DWMWCP_ROUND;
    DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));

    // Match the app background on Win11's 1px window border.
    const D2D1_COLOR_F border = ThemeManager::Instance().GetTokens().windowBackground;
    const COLORREF borderColor = RGB(
        static_cast<int>(border.r * 255.0f),
        static_cast<int>(border.g * 255.0f),
        static_cast<int>(border.b * 255.0f)
    );
    DwmSetWindowAttribute(m_hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

void Window::OnResize(UINT width, UINT height) {
    m_dpiScale = GetDpiScaleForWindow(m_hwnd);
    m_logicalClientSize = Size(
        PhysicalToLogical(static_cast<float>(width), m_hwnd),
        PhysicalToLogical(static_cast<float>(height), m_hwnd)
    );
    m_gfxContext.Resize(width, height);
    m_sceneLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty | RenderLayer::SizeDirty);
    if (m_rootElement) {
        bool isMaximized = IsZoomed(m_hwnd);
        const float resizeBorder = 8.0f / m_dpiScale;

        float padLeft = isMaximized ? resizeBorder : 0.0f;
        float padTop = isMaximized ? resizeBorder : 0.0f;
        float padRight = isMaximized ? resizeBorder : 0.0f;
        float padBottom = resizeBorder;

        const float layoutW = (std::max)(0.0f, PhysicalToLogical(static_cast<float>(width), m_hwnd) - padLeft - padRight);
        const float layoutH = (std::max)(0.0f, PhysicalToLogical(static_cast<float>(height), m_hwnd) - padTop - padBottom);
        Size avail(layoutW, layoutH);
        m_rootElement->Measure(avail);
        m_rootElement->Arrange(Rect(padLeft, padTop, layoutW, layoutH));
        m_popupHost.SetViewport(Rect(padLeft, padTop, layoutW, layoutH));
    }
}

Point Window::ClientPointToLogical(int x, int y) const {
    float lx = 0.0f;
    float ly = 0.0f;
    ClientPhysicalToLogical(m_hwnd, x, y, lx, ly);
    return Point(lx, ly);
}

UIElement* Window::HitTestChrome(float x, float y) const {
    // TitleBar caption band must win over leaked document hits (e.g. scrolled content).
    if (!m_rootElement || y < 0.0f || y > 40.0f) {
        return nullptr;
    }
    for (const auto& child : m_rootElement->GetChildren()) {
        auto* titleBar = dynamic_cast<TitleBar*>(child.get());
        if (!titleBar) {
            continue;
        }
        if (UIElement* hit = titleBar->HitTest(x, y)) {
            return hit;
        }
        if (titleBar->GetBounds().Contains(x, y)) {
            return titleBar;
        }
        break;
    }
    return nullptr;
}

bool Window::OnMouseMove(int x, int y) {
    if (!m_trackingMouse) {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hwnd, 0 };
        TrackMouseEvent(&tme);
        m_trackingMouse = true;
    }

    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    if (auto pressed = LockElement(m_pressedElement)) {
        pressed->OnMouseMove(Point(fx, fy));
        if (NeedsContinuousMouseRedraw(pressed.get())) {
            return true;
        }
        // Thumb-drag / splitter etc. mark local dirty — flush those.
        // Idle press+move (e.g. holding on a rippling nav item) must NOT invalidate.
        if (m_rootElement) {
            DirtyRegion probe;
            m_rootElement->CollectRenderDirtyRegion(probe, false);
            return !probe.IsEmpty();
        }
        return false;
    }

    // Same hit order as LButtonDown (no dismiss): popup → chrome → overlay → tree.
    // ContextMenu hover goes through PopupHost only — not a second HitTestOverlay path.
    UIElement* newHover = m_popupHost.HitTest(fx, fy);
    if (!newHover) {
        newHover = HitTestChrome(fx, fy);
    }
    if (!newHover && m_rootElement) {
        newHover = m_rootElement->HitTestOverlay(fx, fy);
    }
    if (!newHover && m_rootElement) {
        newHover = m_rootElement->HitTest(fx, fy);
    }

    auto hovered = LockElement(m_hoveredElement);
    if (newHover != hovered.get()) {
        if (hovered) {
            hovered->OnMouseLeave();
        }
        SetHoveredElement(newHover);
        hovered = LockElement(m_hoveredElement);
        if (hovered) {
            hovered->OnMouseEnter();
        }
    }

    if (hovered) {
        hovered->OnMouseMove(Point(fx, fy));
        if (auto* titleBar = dynamic_cast<TitleBar*>(hovered.get())) {
            (void)titleBar->ConsumeMenuChromeDirty();
        }
        // Keep m_activeContextMenu synchronized if MenuBar opened a new dropdown
        UIElement* curr = hovered.get();
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu && menu->IsOpen()) {
                m_activeContextMenu = menu;
                break;
            }
            curr = curr->GetParent();
        }
    }

    // Cursor-only / no-chrome hover must NOT Present. Only flush when someone
    // actually MarkRender*Dirty during enter/leave/move.
    if (m_rootElement) {
        DirtyRegion probe;
        m_rootElement->CollectRenderDirtyRegion(probe, false);
        if (!probe.IsEmpty()) {
            return true;
        }
    }
    Rect popupDirty;
    bool hasPopupDirty = false;
    m_popupHost.CollectDirty(popupDirty, hasPopupDirty);
    return hasPopupDirty;
}

bool Window::OnLButtonDown(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    const bool hadContextMenu = m_activeContextMenu && m_activeContextMenu->IsOpen();
    Rect oldMenuBounds = hadContextMenu ? m_activeContextMenu->GetTotalBounds() : Rect();

    auto noteClosedContextMenu = [&]() {
        if (m_activeContextMenu && !m_activeContextMenu->IsOpen()) {
            m_activeContextMenu = nullptr;
            if (!oldMenuBounds.IsEmpty()) {
                m_pendingDirtyRegion.AddRect(oldMenuBounds.Inflate(4.0f));
            }
        }
    };

    auto syncActiveContextMenuFrom = [&](UIElement* from) {
        UIElement* curr = from;
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu && menu->IsOpen()) {
                m_activeContextMenu = menu;
                break;
            }
            curr = curr->GetParent();
        }
    };

    auto pressElement = [&](UIElement* el) {
        SetPressedElement(el);
        SetCapture(m_hwnd);
        el->OnMouseDown(Point(fx, fy));
        noteClosedContextMenu();
        syncActiveContextMenuFrom(el);
    };

    auto applyFocus = [&](UIElement* target) -> bool {
        auto focused = LockElement(m_focusedElement);
        if (target && !target->IsEnabled()) {
            if (focused) {
                focused->OnBlur();
                SetFocusedElement(nullptr);
            }
            return false;
        }
        if (focused && focused.get() != target) {
            focused->OnBlur();
        }
        SetFocusedElement(target);
        focused = LockElement(m_focusedElement);
        if (focused) {
            focused->OnFocus();
        }
        return target != nullptr;
    };

    // 1. PopupHost (ContextMenu / Flyout / pickers) — exclusive; do not also tree-hit.
    if (UIElement* popupHit = m_popupHost.HitTest(fx, fy)) {
        pressElement(popupHit);
        return true;
    }

    // 2. Light-dismiss any open popup when the click is outside.
    if (m_popupHost.DismissIfOutside(fx, fy)) {
        if (hadContextMenu && (!m_activeContextMenu || !m_activeContextMenu->IsOpen())) {
            m_activeContextMenu = nullptr;
            if (!oldMenuBounds.IsEmpty()) {
                m_pendingDirtyRegion.AddRect(oldMenuBounds.Inflate(4.0f));
            }
            // Clear MenuBar active highlight state when context menu is dismissed
            if (m_rootElement) {
                std::function<void(UIElement*)> clearMenuBar = [&](UIElement* elem) {
                    if (!elem) return;
                    if (auto mb = dynamic_cast<MenuBar*>(elem)) {
                        mb->ResetInteractionState();
                    }
                    for (auto& child : elem->GetChildren()) {
                        clearMenuBar(child.get());
                    }
                };
                clearMenuBar(m_rootElement.get());
            }
            if (auto hovered = LockElement(m_hoveredElement)) {
                hovered->OnMouseLeave();
            }
            m_hoveredElement.reset();
        }
    }

    // 3. Chrome / TitleBar band (toggles handled inside TitleBar::OnMouseDown).
    if (UIElement* chromeHit = HitTestChrome(fx, fy)) {
        if (applyFocus(chromeHit)) {
            pressElement(chromeHit);
        }
        return true;
    }

    // 4. Non-popup overlays still on the visual tree.
    if (m_rootElement) {
        if (UIElement* overlayHit = m_rootElement->HitTestOverlay(fx, fy)) {
            pressElement(overlayHit);
            return true;
        }
    }

    // 5. Document tree.
    UIElement* target = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
    if (applyFocus(target) && target) {
        pressElement(target);
    }
    return target != nullptr || m_activeContextMenu != nullptr;
}

void Window::OnLButtonDblClick(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    UIElement* target = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
    if (target && target->IsEnabled()) {
        target->OnMouseDblClick(Point(fx, fy));
    }
}

bool Window::OnLButtonUp(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;
    bool dirty = false;

    if (auto pressed = LockElement(m_pressedElement)) {
        pressed->OnMouseUp(Point(fx, fy));
        dirty = true;
    }
    m_pressedElement.reset();
    ReleaseCapture();
    return dirty;
}

void Window::OnRButtonDown(int x, int y) {
    Point logicalPt = ClientPointToLogical(x, y);
    float fx = logicalPt.x;
    float fy = logicalPt.y;

    if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
        m_activeContextMenu->Hide();
        m_activeContextMenu = nullptr;
    }

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    (void)rc;

    UIElement* target = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
    if (target) {
        auto focused = LockElement(m_focusedElement);
        if (focused && focused.get() != target) {
            focused->OnBlur();
        }
        SetFocusedElement(target);
        if (auto curFocused = LockElement(m_focusedElement)) {
            curFocused->OnFocus();
        }

        target->OnMouseRightClick(Point(fx, fy));

        UIElement* curr = target;
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu) {
                m_activeContextMenu = menu;
                m_activeContextMenu->ShowAt(fx, fy);
                break;
            }
            curr = curr->GetParent();
        }
    }
}

} // namespace CUI
