#include "Window.h"
#include "../style/ThemeManager.h"
#include "../parser/StyleManager.h"
#include "../controls/TextBox.h"
#include "../controls/ContextMenu.h"
#include "../controls/VSCodeControls.h"
#include "../controls/DatePicker.h"
#include "../controls/TimePicker.h"
#include "../controls/ColorPicker.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <imm.h>
#include <algorithm>
#include <chrono>
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

float GetWindowRefreshRateHz(HWND hwnd) {
    if (!hwnd) {
        return 60.0f;
    }

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
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
    if (hz == 0 || hz == 1) {
        return 60.0f;
    }
    return static_cast<float>(hz);
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

void ApplyThemeToken(UIElement* element, const char* tokenPropName, const char* targetPropName) {
    if (!element || !element->HasProperty(tokenPropName)) {
        return;
    }
    const std::string tokenName = element->GetProperty(tokenPropName).AsString();
    if (tokenName.empty()) {
        return;
    }
    element->SetProperty(targetPropName, Value(ThemeManager::Instance().GetColor(tokenName)));
}

void ApplyThemeToTree(UIElement* element, bool systemBackdrop) {
    if (!element) {
        return;
    }

    ApplyThemeToken(element, "theme.backgroundToken", "background");
    ApplyThemeToken(element, "theme.hoverBackgroundToken", "hoverBackground");
    ApplyThemeToken(element, "theme.pressedBackgroundToken", "pressedBackground");
    ApplyThemeToken(element, "theme.borderToken", "borderBrush");
    ApplyThemeToken(element, "theme.focusedBorderToken", "focusedBorderBrush");
    ApplyThemeToken(element, "theme.colorToken", "color");
    ApplyThemeToken(element, "theme.placeholderColorToken", "placeholderColor");
    ApplyThemeToken(element, "theme.dropdownBackgroundToken", "dropdownBackground");
    ApplyThemeToken(element, "theme.selectedItemBackgroundToken", "selectedItemBackground");
    ApplyThemeToken(element, "theme.selectedBackgroundToken", "selectedBackground");
    ApplyThemeToken(element, "theme.headerBackgroundToken", "headerBackground");
    ApplyThemeToken(element, "theme.activeTabBackgroundToken", "activeTabBackground");
    ApplyThemeToken(element, "theme.inactiveTabBackgroundToken", "inactiveTabBackground");
    ApplyThemeToken(element, "theme.underlineColorToken", "underlineColor");
    ApplyThemeToken(element, "theme.activeUnderlineColorToken", "activeUnderlineColor");
    ApplyThemeToken(element, "theme.disabledBackgroundToken", "disabledBackground");
    ApplyThemeToken(element, "theme.activeColorToken", "activeColor");
    ApplyThemeToken(element, "theme.fillColorToken", "fillColor");
    ApplyThemeToken(element, "theme.trackColorToken", "trackColor");
    ApplyThemeToken(element, "theme.activeTrackColorToken", "activeTrackColor");
    ApplyThemeToken(element, "theme.thumbColorToken", "thumbColor");
    ApplyThemeToken(element, "theme.onColorToken", "onColor");
    ApplyThemeToken(element, "theme.offColorToken", "offColor");
    ApplyThemeToken(element, "theme.knobColorToken", "knobColor");
    ApplyThemeToken(element, "theme.paneBackgroundToken", "paneBackground");
    ApplyThemeToken(element, "theme.indicatorColorToken", "indicatorColor");
    ApplyThemeToken(element, "theme.accentToken", "accent");
    ApplyThemeToken(element, "theme.accentColorToken", "accentColor");
    ApplyThemeToken(element, "theme.checkedBackgroundToken", "checkedBackground");
    ApplyThemeToken(element, "theme.gridLineBrushToken", "gridLineBrush");
    ApplyThemeToken(element, "theme.titleColorToken", "titleColor");
    ApplyThemeToken(element, "theme.messageColorToken", "messageColor");

    const std::string className = element->GetClassName();
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    if (className == "TitleBar") {
        // Keep chrome fill stable across hover/focus; always sync from titleBarBackground.
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("theme.backgroundToken", Value("titleBarBackground"));
        }
        if (!element->HasProperty("theme.hoverBackgroundToken")) {
            element->SetProperty("theme.hoverBackgroundToken", Value("titleBarBackground"));
        }
        if (!element->HasProperty("theme.pressedBackgroundToken")) {
            element->SetProperty("theme.pressedBackgroundToken", Value("titleBarBackground"));
        }
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("theme.colorToken", Value("titleBarText"));
        }
        element->SetProperty("background", Value(tokens.titleBarBackground));
        element->SetProperty("hoverBackground", Value(tokens.titleBarBackground));
        element->SetProperty("pressedBackground", Value(tokens.titleBarBackground));
        element->SetProperty("color", Value(tokens.titleBarText));
        // Let Mica/Acrylic show through the title bar chrome.
        element->SetProperty("chromeBackdropAlpha", Value(systemBackdrop ? 0.28f : 1.0f));
    } else if (className == "MenuBar") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.titleBarBackground));
        }
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.titleBarText));
        }
    } else if (className == "Button") {
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.accentForeground));
        }
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.hoverBackgroundToken")) {
            element->SetProperty("hoverBackground", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.pressedBackgroundToken")) {
            element->SetProperty("pressedBackground", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.focusedBorderToken")) {
            element->SetProperty("focusedBorderBrush", Value(tokens.focusedBorder));
        }
    } else if (className == "PropertyGrid") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.cardBackground));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.cardBorder));
        }
    } else if (className == "ContextMenu") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.cardBackground));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.cardBorder));
        }
    } else if (className == "MenuItem") {
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textSecondary));
        }
    } else if (className == "TextBox") {
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textPrimary));
        }
        if (!element->HasProperty("theme.placeholderColorToken")) {
            element->SetProperty("placeholderColor", Value(tokens.textMuted));
        }
        if (!element->HasProperty("theme.activeUnderlineColorToken")) {
            element->SetProperty("activeUnderlineColor", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.underlineColorToken")) {
            element->SetProperty("underlineColor", Value(tokens.cardBorder));
        }
    } else if (className == "ComboBox") {
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textPrimary));
        }
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.inputBackground));
        }
        if (!element->HasProperty("theme.hoverBackgroundToken")) {
            element->SetProperty("hoverBackground", Value(tokens.hoverBackground));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.inputBorder));
        }
        if (!element->HasProperty("theme.focusedBorderToken")) {
            element->SetProperty("focusedBorderBrush", Value(tokens.focusedBorder));
        }
        if (!element->HasProperty("theme.dropdownBackgroundToken")) {
            element->SetProperty("dropdownBackground", Value(tokens.cardBackground));
        }
        if (!element->HasProperty("theme.selectedItemBackgroundToken")) {
            element->SetProperty("selectedItemBackground", Value(tokens.accentColor));
        }
    } else if (className == "CheckBox") {
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textPrimary));
        }
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.inputBackground));
        }
        if (!element->HasProperty("theme.checkedBackgroundToken") && !element->HasProperty("checkedBackground")) {
            element->SetProperty("checkedBackground", Value(tokens.accentColor));
        }
    } else if (className == "RadioButton") {
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textPrimary));
        }
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.inputBackground));
        }
        if (!element->HasProperty("theme.accentColorToken")) {
            element->SetProperty("accentColor", Value(tokens.accentColor));
        }
    } else if (className == "ToggleSwitch") {
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textPrimary));
        }
        if (!element->HasProperty("theme.onColorToken")) {
            element->SetProperty("onColor", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.offColorToken")) {
            element->SetProperty("offColor", Value(tokens.inputBorder));
        }
        if (!element->HasProperty("theme.knobColorToken")) {
            element->SetProperty("knobColor", Value(tokens.accentForeground));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.cardBorder));
        }
        if (!element->HasProperty("theme.hoverBackgroundToken")) {
            element->SetProperty("hoverBackground", Value(tokens.hoverBackground));
        }
        if (!element->HasProperty("theme.pressedBackgroundToken")) {
            element->SetProperty("pressedBackground", Value(tokens.pressedBackground));
        }
    } else if (className == "Slider") {
        if (!element->HasProperty("theme.trackColorToken")) {
            element->SetProperty("trackColor", Value(tokens.cardBorder));
        }
        if (!element->HasProperty("theme.activeTrackColorToken")) {
            element->SetProperty("activeTrackColor", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.thumbColorToken")) {
            element->SetProperty("thumbColor", Value(tokens.textPrimary));
        }
    } else if (className == "CollapsePanel") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.cardBackground));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.cardBorder));
        }
    } else if (className == "ListBox") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.cardBackground));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.cardBorder));
        }
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textSecondary));
        }
        if (!element->HasProperty("theme.selectedBackgroundToken")) {
            element->SetProperty("selectedBackground", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.hoverBackgroundToken")) {
            element->SetProperty("hoverBackground", Value(tokens.hoverBackground));
        }
    } else if (className == "ListView") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.windowBackground));
        }
        if (!element->HasProperty("theme.headerBackgroundToken")) {
            element->SetProperty("headerBackground", Value(tokens.paneBackground));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.cardBorder));
        }
        if (!element->HasProperty("theme.gridLineBrushToken")) {
            element->SetProperty("gridLineBrush", Value(tokens.inputBorder));
        }
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textSecondary));
        }
        if (!element->HasProperty("theme.selectedBackgroundToken")) {
            element->SetProperty("selectedBackground", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.hoverBackgroundToken")) {
            element->SetProperty("hoverBackground", Value(tokens.hoverBackground));
        }
    } else if (className == "TabView") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.windowBackground));
        }
        if (!element->HasProperty("theme.headerBackgroundToken")) {
            element->SetProperty("headerBackground", Value(tokens.paneBackground));
        }
        if (!element->HasProperty("theme.activeTabBackgroundToken")) {
            element->SetProperty("activeTabBackground", Value(tokens.windowBackground));
        }
        if (!element->HasProperty("theme.inactiveTabBackgroundToken")) {
            element->SetProperty("inactiveTabBackground", Value(tokens.cardBackground));
        }
        if (!element->HasProperty("theme.underlineColorToken")) {
            element->SetProperty("underlineColor", Value(tokens.cardBorder));
        }
        if (!element->HasProperty("theme.activeUnderlineColorToken")) {
            element->SetProperty("activeUnderlineColor", Value(tokens.accentColor));
        }
    } else if (className == "TreeView") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.cardBackground));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.cardBorder));
        }
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textSecondary));
        }
        if (!element->HasProperty("theme.selectedBackgroundToken")) {
            element->SetProperty("selectedBackground", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.hoverBackgroundToken")) {
            element->SetProperty("hoverBackground", Value(tokens.hoverBackground));
        }
    } else if (className == "BreadcrumbBar") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.cardBackground));
        }
        if (!element->HasProperty("theme.borderToken")) {
            element->SetProperty("borderBrush", Value(tokens.cardBorder));
        }
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.textSecondary));
        }
        if (!element->HasProperty("theme.activeColorToken")) {
            element->SetProperty("activeColor", Value(tokens.accentColor));
        }
    } else if (className == "ProgressBar") {
        if (!element->HasProperty("theme.fillColorToken")) {
            element->SetProperty("fillColor", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.trackColorToken")) {
            element->SetProperty("trackColor", Value(tokens.cardBorder));
        }
    } else if (className == "NavigationView") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.windowBackground));
        }
        if (!element->HasProperty("theme.paneBackgroundToken")) {
            element->SetProperty("paneBackground", Value(tokens.paneBackground));
        }
        if (!element->HasProperty("theme.indicatorColorToken")) {
            element->SetProperty("indicatorColor", Value(tokens.accentColor));
        }
        // Transparent host + translucent pane so system backdrop can show through chrome.
        element->SetProperty("chromeBackdropAlpha", Value(systemBackdrop ? 0.0f : 1.0f));
        element->SetProperty("paneBackdropAlpha", Value(systemBackdrop ? 0.32f : 1.0f));
    } else if (className == "NavigationViewItem" || className == "NavigationViewItemHeader") {
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("theme.colorToken", Value("textPrimary"));
        }
        element->SetProperty("color", Value(tokens.textPrimary));
        if (element->HasProperty("theme.hoverBackgroundToken")) {
            element->SetProperty("hoverBackground", Value(tokens.hoverBackground));
        }
        if (element->HasProperty("theme.selectedBackgroundToken")) {
            element->SetProperty("selectedBackground", Value(tokens.selectedBackground));
        }
        // Keep item chips translucent so pane mica is not fully covered.
        element->SetProperty("chromeBackdropAlpha", Value(systemBackdrop ? 0.55f : 1.0f));
    } else if (className == "TextBlock" || className == "HyperlinkButton") {
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(className == "HyperlinkButton" ? tokens.accentColor : tokens.textSecondary));
        }
    } else if (className == "ActivityBar") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.activityBarBackground));
        }
    } else if (className == "SideBar" || className == "TabBar") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.paneBackground));
        }
    } else if (className == "EditorView") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.windowBackground));
        }
    } else if (className == "StatusBar") {
        if (!element->HasProperty("theme.backgroundToken")) {
            element->SetProperty("background", Value(tokens.accentColor));
        }
        if (!element->HasProperty("theme.colorToken")) {
            element->SetProperty("color", Value(tokens.accentForeground));
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

    element->SetProperty("theme.refreshNonce", Value(refreshStamp));

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
    if (!element) {
        return false;
    }

    const std::string className = element->GetClassName();
    return className == "TitleBar" || className == "MenuBar";
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
    m_rootElement->CollectAnimationBounds(dirtyRect, hasDirty);

    if (m_hasLastAnimationDirtyRect) {
        dirtyRect = hasDirty ? dirtyRect.Union(m_lastAnimationDirtyRect) : m_lastAnimationDirtyRect;
        hasDirty = true;
    }

    if (!hasDirty) {
        return;
    }

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
}

Window::~Window() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

bool Window::Create(const std::string& title, int width, int height, bool transparentMode) {
    m_transparentMode = transparentMode;
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"CUI_WindowClass";

    RegisterClassEx(&wc);

    std::wstring wTitle(title.begin(), title.end());

    // DirectComposition host: HWND DXGI swap chains ignore per-pixel alpha, so
    // system Mica/Acrylic never shows through. Composition swap chains do.
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
#ifndef WS_EX_NOREDIRECTIONBITMAP
#define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif
    DWORD dwExStyle = WS_EX_NOREDIRECTIONBITMAP;
    if (m_transparentMode) {
        dwExStyle |= (WS_EX_LAYERED | WS_EX_APPWINDOW);
    }

    m_hwnd = CreateWindowEx(
        dwExStyle,
        L"CUI_WindowClass",
        wTitle.c_str(),
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        hInstance,
        this
    );

    if (!m_hwnd) return false;

    UpdateDwmChrome();
    WindowBackdrop::ApplyBackdrop(m_hwnd, m_backdropType);
    WindowBackdrop::ApplyTheme(m_hwnd, m_themeMode);

    if (m_transparentMode) {
        SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
    }

    if (!m_gfxContext.Initialize(m_hwnd)) {
        return false;
    }

    return true;
}

void Window::SetBackdropType(BackdropType type) {
    m_backdropType = type;
    if (m_hwnd) {
        WindowBackdrop::ApplyBackdrop(m_hwnd, type);
        ApplyVisualState();
        RequestFullRepaint();
    }
}

void Window::SetThemeMode(ThemeMode theme) {
    m_themeMode = theme;
    ThemeManager::Instance().SetThemeMode(theme);
    StyleManager::Instance().ReloadFromTheme();
    if (m_hwnd) {
        WindowBackdrop::ApplyTheme(m_hwnd, theme);
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
    auto lastFrameTime = clock::now();
    bool animationActive = false;

    for (;;) {
        bool hadMessage = false;
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            hadMessage = true;
            if (msg.message == WM_QUIT) {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        const bool wasAnimationActive = animationActive;
        const auto now = clock::now();
        const float refreshHz = GetWindowRefreshRateHz(m_hwnd);
        const double targetFps = m_lowPerformanceMode ? 8.0 : static_cast<double>((std::max)(30.0f, refreshHz));
        const auto targetFrame = std::chrono::duration<double>(1.0 / targetFps);
        m_animationManager.SetTargetFrameSeconds(static_cast<float>(targetFrame.count()));
        const bool shouldProbeAnimation = hadMessage || animationActive;
        const bool frameDue = !animationActive || (now - lastFrameTime) >= targetFrame;
        bool animating = animationActive;
        bool didAnimationTick = false;
        if (shouldProbeAnimation && frameDue && m_rootElement) {
            m_animationManager.BeginFrame(now, animationActive);
            UIElement::SetAnimationDeltaSeconds(m_animationManager.GetDeltaSeconds());
            animating = m_rootElement->OnAnimationTick();
            if (auto focused = LockElement(m_focusedElement)) {
                if (focused->NeedsAutoScrollTick()) {
                    focused->OnAutoScrollTick();
                    animating = true;
                }
            }
            lastFrameTime = now;
            didAnimationTick = true;
        }
        animationActive = animating;

        if (didAnimationTick && (animating || wasAnimationActive)) {
            InvalidateAnimatedRegions(animating);
        }

        if (!hadMessage) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
                continue;
            }
            if (animationActive) {
                auto elapsed = std::chrono::duration<double>(clock::now() - lastFrameTime);
                DWORD waitMs = elapsed >= targetFrame
                    ? 0
                    : static_cast<DWORD>(std::ceil(std::chrono::duration<double, std::milli>(targetFrame - elapsed).count()));
                MsgWaitForMultipleObjectsEx(0, nullptr, waitMs, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            } else {
                WaitMessage();
            }
        }
    }
}

void Window::SetRootElement(std::shared_ptr<UIElement> root) {
    m_rootElement = root;
    if (m_rootElement) {
        ApplyVisualState();
        m_rootElement->SyncRenderState();
    }
    Relayout();
}

void Window::ApplyVisualState() {
    const bool systemBackdrop = (m_backdropType != BackdropType::None);
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
        float fx = static_cast<float>(clientPt.x);
        float fy = static_cast<float>(clientPt.y);

        RECT rc;
        GetClientRect(m_hwnd, &rc);
        float winW = static_cast<float>(rc.right);
        float winH = static_cast<float>(rc.bottom);

        // TitleBar chrome toggles / menu must win over resize borders and caption drag.
        // Prefer direct TitleBar child lookup — tree HitTest can still be wrong if content leaks.
        if (fy >= 0 && fy <= 40.0f && fx < winW - 135.0f && m_rootElement) {
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

        // System buttons get first chance after chrome toggles.
        if (fy >= 0 && fy <= 40.0f) {
            if (fx >= winW - 45.0f) {
                return HTCLOSE;
            }
            if (fx >= winW - 90.0f && fx < winW - 45.0f) {
                return HTMAXBUTTON;
            }
            if (fx >= winW - 135.0f && fx < winW - 90.0f) {
                return HTMINBUTTON;
            }
        }

        // 2. Resizing border handles (8-direction border resize) - only when NOT maximized
        if (!IsZoomed(m_hwnd)) {
            int borderThickness = 8;
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
            if (fx < winW - 135.0f && m_rootElement) {
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

    case WM_CUI_TOGGLE_LOW_PERF:
        SetLowPerformanceMode(!m_lowPerformanceMode);
        return 0;

    case WM_CUI_TOGGLE_BACKDROP:
        switch (m_backdropType) {
        case BackdropType::Mica: SetBackdropType(BackdropType::MicaAlt); break;
        case BackdropType::MicaAlt: SetBackdropType(BackdropType::Acrylic); break;
        case BackdropType::Acrylic: SetBackdropType(BackdropType::None); break;
        case BackdropType::None: default: SetBackdropType(BackdropType::Mica); break;
        }
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
            const bool dragging = !m_pressedElement.expired();
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

            if (moved || menuBoundsChanged || !oldMenuBounds.IsEmpty()) {
                if (!oldMenuBounds.IsEmpty()) {
                    m_pendingDirtyRegion.AddRect(oldMenuBounds.Inflate(4.0f));
                }
                if (!newMenuBounds.IsEmpty()) {
                    m_pendingDirtyRegion.AddRect(newMenuBounds.Inflate(4.0f));
                }
                if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
                    InvalidatePendingRenderRegions(true);
                } else if (dragging) {
                    InvalidatePendingRenderRegions(false);
                } else {
                    InvalidatePendingRenderRegions(true);
                }
            }
        }
        return 0;

    case WM_NCMOUSEMOVE:
        {
            // Narrow DWM handoff: enough for non-client hover behavior, without
            // letting DWM resurrect native caption painting via NCCALCSIZE/PAINT.
            LRESULT dwmResult = 0;
            DwmDefWindowProc(m_hwnd, uMsg, wParam, lParam, &dwmResult);
        }
        RequestFullRepaint();
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

    case WM_NCMOUSELEAVE:
        RequestFullRepaint();
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
        float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        float fx = static_cast<float>(pt.x);
        float fy = static_cast<float>(pt.y);

        UIElement* target = nullptr;
        if (m_rootElement) {
            target = m_rootElement->HitTestOverlay(fx, fy);
            if (!target) {
                target = m_rootElement->HitTest(fx, fy);
            }
        }
        if (target) {
            target->OnMouseWheel(delta);
            InvalidatePendingRenderRegions(true);
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_F8) {
            m_showRenderStatsOverlay = !m_showRenderStatsOverlay;
            RequestFullRepaint();
            return 0;
        }
        if (auto focused = LockElement(m_focusedElement)) {
            if (focused->IsEnabled()) {
                focused->OnKeyDown(static_cast<int>(wParam));
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
            InvalidatePendingRenderRegions(true);
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

    Rect paintBounds(
        static_cast<float>(ps.rcPaint.left),
        static_cast<float>(ps.rcPaint.top),
        static_cast<float>(ps.rcPaint.right - ps.rcPaint.left),
        static_cast<float>(ps.rcPaint.bottom - ps.rcPaint.top)
    );
    m_gfxContext.SetPaintBounds(paintBounds);
    DirtyRegion frameDirtyRegion = m_pendingDirtyRegion;
    if (frameDirtyRegion.IsEmpty()) {
        frameDirtyRegion.AddRect(paintBounds);
    }

    Rect viewportBounds = GetClientBounds(m_hwnd);
    Rect dirtyBounds = frameDirtyRegion.GetBounds();
    bool fullRepaint = frameDirtyRegion.GetRectCount() == 0
        || viewportBounds.IsEmpty()
        || CoversRect(paintBounds, viewportBounds)
        || CoversRect(dirtyBounds, viewportBounds)
        || !m_sceneLayer.IsValid();

    m_compositionContext.BeginFrame(viewportBounds, frameDirtyRegion, fullRepaint);
    m_gfxContext.SetCompositionContext(&m_compositionContext);

    m_gfxContext.BeginDraw();

    const D2D1_COLOR_F bgColor = ThemeManager::Instance().GetTokens().windowBackground;
    const bool systemBackdrop = (m_backdropType != BackdropType::None);
    // Transparent clear is required for DWM Mica/Acrylic to show through chrome.
    const D2D1_COLOR_F sceneClearColor = (systemBackdrop || (m_transparentMode && !IsZoomed(m_hwnd)))
        ? D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)
        : bgColor;

    const Size sceneSize(viewportBounds.width, viewportBounds.height);
    const bool sceneSizeChanged =
        std::abs(m_sceneLayer.GetCacheSurfaceSize().width - sceneSize.width) > 0.5f
        || std::abs(m_sceneLayer.GetCacheSurfaceSize().height - sceneSize.height) > 0.5f;
    const bool canRestoreScene =
        !fullRepaint
        && !sceneSizeChanged
        && m_sceneLayer.IsValid()
        && m_sceneLayer.GetCacheBitmap() != nullptr
        && m_sceneLayer.GetScratchBitmap() != nullptr;

    if (m_gfxContext.PushLayerTarget(m_sceneLayer, sceneSize, dirtyBounds.IsEmpty() ? viewportBounds : dirtyBounds, sceneClearColor)) {
        auto* sceneContext = m_gfxContext.GetD2DContext();
        if (canRestoreScene) {
            ID2D1Bitmap1* snapshot = m_sceneLayer.GetScratchBitmap();
            snapshot->CopyFromBitmap(nullptr, m_sceneLayer.GetCacheBitmap(), nullptr);
            sceneContext->DrawBitmap(
                snapshot,
                viewportBounds.ToD2D(),
                1.0f,
                D2D1_INTERPOLATION_MODE_LINEAR,
                nullptr
            );
        }

        auto renderScene = [&]() {
            if (m_rootElement) {
                m_rootElement->Render(m_gfxContext);
                m_rootElement->RenderOverlay(m_gfxContext);
            }
            if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
                m_activeContextMenu->OnRenderOverlay(m_gfxContext);
            }
        };

        if (canRestoreScene && frameDirtyRegion.GetRectCount() > 0) {
            for (const auto& rect : frameDirtyRegion.GetRects()) {
                if (rect.IsEmpty()) {
                    continue;
                }
                m_gfxContext.PushClip(rect);
                if (sceneClearColor.a > 0.0f) {
                    m_gfxContext.FillRect(rect, sceneClearColor);
                } else {
                    sceneContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
                }
                renderScene();
                m_gfxContext.PopClip();
            }
        } else {
            renderScene();
        }

        m_gfxContext.PopLayerTarget(m_sceneLayer);
        m_sceneLayer.Validate();
    }

    if (systemBackdrop || (m_transparentMode && !IsZoomed(m_hwnd))) {
        m_gfxContext.GetD2DContext()->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    } else {
        m_gfxContext.GetD2DContext()->Clear(bgColor);
    }
    m_gfxContext.DrawLayer(m_sceneLayer, viewportBounds);
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
        << "  显示帧率: " << static_cast<int>(std::round(m_overlayFps));

    const std::string text = ss.str();
    Rect panel(12.0f, (std::max)(12.0f, m_rootElement ? (m_rootElement->GetBounds().height - 40.0f) : 12.0f), 760.0f, 28.0f);
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
    const MARGINS margins = maximized
        ? MARGINS{ 0, 0, 0, 0 }
        : (systemBackdrop ? MARGINS{ -1, -1, -1, -1 } : MARGINS{ 1, 1, 1, 1 });
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

    // Register Custom TitleBar Maximize Button Rect with Win11 DWM (DWMWA_CAPTION_BUTTON_BOUNDS = 35)
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    float winW = static_cast<float>(rc.right);
    RECT maxBtnRect = {
        static_cast<LONG>(winW - 90.0f),
        0,
        static_cast<LONG>(winW - 45.0f),
        static_cast<LONG>(40.0f)
    };
    DwmSetWindowAttribute(m_hwnd, 35, &maxBtnRect, sizeof(maxBtnRect));
}

void Window::OnResize(UINT width, UINT height) {
    m_gfxContext.Resize(width, height);
    m_sceneLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty | RenderLayer::SizeDirty);
    if (m_rootElement) {
        bool isMaximized = IsZoomed(m_hwnd);
        constexpr float resizeBorder = 8.0f;

        // Custom chrome makes the client rect equal to the whole window so DWM
        // snap/resize behavior works. The bottom few pixels are still the resize
        // edge, though, and are not a reliable viewport for scrollable content.
        // Keep them outside the layout rect so ScrollViewer computes max scroll
        // from the actually visible area instead of hiding the last row under
        // the resize edge.
        float padLeft = isMaximized ? resizeBorder : 0.0f;
        float padTop = isMaximized ? resizeBorder : 0.0f;
        float padRight = isMaximized ? resizeBorder : 0.0f;
        float padBottom = resizeBorder;

        float layoutW = (std::max)(0.0f, static_cast<float>(width) - padLeft - padRight);
        float layoutH = (std::max)(0.0f, static_cast<float>(height) - padTop - padBottom);
        Size avail(layoutW, layoutH);
        m_rootElement->Measure(avail);
        m_rootElement->Arrange(Rect(padLeft, padTop, layoutW, layoutH));
    }
}

bool Window::OnMouseMove(int x, int y) {
    bool dirty = false;
    if (!m_trackingMouse) {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hwnd, 0 };
        TrackMouseEvent(&tme);
        m_trackingMouse = true;
    }

    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
        UIElement* itemHover = m_activeContextMenu->HitTestOverlay(fx, fy);
        if (!itemHover && m_rootElement) {
            // Check if hovering over MenuBar items while dropdown is active
            UIElement* rootHit = m_rootElement->HitTest(fx, fy);
            if (rootHit) {
                itemHover = rootHit;
            }
        }

        auto hovered = LockElement(m_hoveredElement);
        if (itemHover != hovered.get()) {
            if (hovered) hovered->OnMouseLeave();
            SetHoveredElement(itemHover);
            hovered = LockElement(m_hoveredElement);
            if (hovered) hovered->OnMouseEnter();
            dirty = true;
        }
        if (hovered) {
            hovered->OnMouseMove(Point(fx, fy));
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
        return dirty || NeedsContinuousMouseRedraw(hovered.get());
    }

    if (auto pressed = LockElement(m_pressedElement)) {
        pressed->OnMouseMove(Point(fx, fy));
        return true;
    }

    UIElement* newHover = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
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
        dirty = true;
    }

    if (hovered) {
        hovered->OnMouseMove(Point(fx, fy));
        dirty = dirty || NeedsContinuousMouseRedraw(hovered.get());
    }
    return dirty;
}

bool Window::OnLButtonDown(int x, int y) {
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    if (m_rootElement) {
        UIElement* overlayHit = m_rootElement->HitTestOverlay(fx, fy);
        if (overlayHit) {
            SetPressedElement(overlayHit);
            SetCapture(m_hwnd);
            overlayHit->OnMouseDown(Point(fx, fy));
            return true;
        } else {
            // Clicked outside any overlay popups -> close open DatePicker/TimePicker/ColorPicker popups
            std::function<void(UIElement*)> closePopups = [&](UIElement* elem) {
                if (!elem) return;
                if (auto dp = dynamic_cast<DatePicker*>(elem)) dp->SetPopupOpen(false);
                if (auto tp = dynamic_cast<TimePicker*>(elem)) tp->SetPopupOpen(false);
                if (auto cp = dynamic_cast<ColorPicker*>(elem)) cp->SetPopupOpen(false);
                for (auto& child : elem->GetChildren()) {
                    closePopups(child.get());
                }
            };
            closePopups(m_rootElement.get());
        }
    }

    if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
        Rect oldMenuBounds = m_activeContextMenu->GetTotalBounds();
        UIElement* menuHit = m_activeContextMenu->HitTestOverlay(fx, fy);
        if (menuHit) {
            menuHit->OnMouseDown(Point(fx, fy));
            m_activeContextMenu = nullptr;
            if (!oldMenuBounds.IsEmpty()) {
                m_pendingDirtyRegion.AddRect(oldMenuBounds.Inflate(4.0f));
            }
            return true;
        } else {
            m_activeContextMenu->Hide();
            m_activeContextMenu = nullptr;
            if (!oldMenuBounds.IsEmpty()) {
                m_pendingDirtyRegion.AddRect(oldMenuBounds.Inflate(4.0f));
            }
            // Clear MenuBar active highlight state when clicking outside
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

    UIElement* target = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;

    // TitleBar chrome must win over any leaked content hits in the top band
    // (ScrollViewer used to let scrolled PropertyGrid children steal these clicks).
    if (m_rootElement && fy >= 0.0f && fy <= 40.0f) {
        for (const auto& child : m_rootElement->GetChildren()) {
            auto* titleBar = dynamic_cast<TitleBar*>(child.get());
            if (!titleBar) {
                continue;
            }
            if (titleBar->IsLowPerformanceToggleHit(fx, fy)) {
                PostMessageW(m_hwnd, WM_CUI_TOGGLE_LOW_PERF, 0, 0);
                return true;
            }
            if (titleBar->IsBackdropToggleHit(fx, fy)) {
                PostMessageW(m_hwnd, WM_CUI_TOGGLE_BACKDROP, 0, 0);
                return true;
            }
            if (titleBar->IsThemeToggleHit(fx, fy)) {
                PostMessageW(m_hwnd, WM_CUI_TOGGLE_THEME, 0, 0);
                return true;
            }
            if (titleBar->GetBounds().Contains(fx, fy)) {
                target = titleBar;
            }
            break;
        }
    }

    // Legacy path when HitTest already returned TitleBar*
    if (auto* titleBar = dynamic_cast<TitleBar*>(target)) {
        if (titleBar->IsLowPerformanceToggleHit(fx, fy)) {
            PostMessageW(m_hwnd, WM_CUI_TOGGLE_LOW_PERF, 0, 0);
            return true;
        }
        if (titleBar->IsBackdropToggleHit(fx, fy)) {
            PostMessageW(m_hwnd, WM_CUI_TOGGLE_BACKDROP, 0, 0);
            return true;
        }
        if (titleBar->IsThemeToggleHit(fx, fy)) {
            PostMessageW(m_hwnd, WM_CUI_TOGGLE_THEME, 0, 0);
            return true;
        }
    }

    auto focused = LockElement(m_focusedElement);
    if (target && !target->IsEnabled()) {
        if (focused) {
            focused->OnBlur();
            SetFocusedElement(nullptr);
        }
        return true;
    }

    if (focused && focused.get() != target) {
        focused->OnBlur();
    }
    SetFocusedElement(target);
    focused = LockElement(m_focusedElement);
    if (focused) {
        focused->OnFocus();
    }

    if (target) {
        SetPressedElement(target);
        SetCapture(m_hwnd);
        if (auto pressed = LockElement(m_pressedElement)) {
            pressed->OnMouseDown(Point(fx, fy));
        }

        // Check if target or ancestor activated a ContextMenu
        UIElement* curr = target;
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu && menu->IsOpen()) {
                m_activeContextMenu = menu;
                break;
            }
            curr = curr->GetParent();
        }
    }
    return target != nullptr || m_activeContextMenu != nullptr;
}

void Window::OnLButtonDblClick(int x, int y) {
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    UIElement* target = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
    if (target && target->IsEnabled()) {
        target->OnMouseDblClick(Point(fx, fy));
    }
}

bool Window::OnLButtonUp(int x, int y) {
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);
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
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
        m_activeContextMenu->Hide();
        m_activeContextMenu = nullptr;
    }

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    float winW = static_cast<float>(rc.right);
    float winH = static_cast<float>(rc.bottom);

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
                m_activeContextMenu->ShowAt(fx, fy, winW, winH);
                break;
            }
            curr = curr->GetParent();
        }
    }
}

} // namespace CUI
