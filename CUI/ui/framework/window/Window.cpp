#include "Window.h"
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
    InvalidateRect(m_hwnd, nullptr, FALSE);
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
        KillTimer(m_hwnd, 1);
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

    // Standard OVERLAPPEDWINDOW with DWM non-client frame extension for 100% native OS window animations
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
    DWORD dwExStyle = m_transparentMode ? (WS_EX_LAYERED | WS_EX_APPWINDOW) : 0;

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

    if (m_transparentMode) {
        SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
    }

    if (!m_gfxContext.Initialize(m_hwnd)) {
        return false;
    }

    SetTimer(m_hwnd, 1, 500, nullptr);

    return true;
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
        const auto targetFrame = std::chrono::duration<double>(1.0 / (std::max)(30.0f, refreshHz));
        m_animationManager.SetTargetFrameSeconds(static_cast<float>(targetFrame.count()));
        const bool shouldProbeAnimation = hadMessage || (!m_lowPerformanceMode && animationActive);
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
        m_rootElement->SyncRenderState();
    }
    Relayout();
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

        // System buttons get first chance. In windowed mode, the top resize band
        // overlaps their upper pixels; returning HTTOP there prevents Win11 from
        // seeing HTMAXBUTTON, so Snap Layout hover never appears.
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
                    if (titleBar->IsLowPerformanceToggleHit(fx, fy)) {
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

    case WM_TIMER:
        if (wParam == 1) {
            if (auto focused = LockElement(m_focusedElement)) {
                RequestFullRepaint();
            }
        }
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
            if (OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
                if (dragging) {
                    InvalidatePendingRenderRegions(false);
                } else {
                    InvalidateAnimatedRegions(true);
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
            focused->OnKeyDown(static_cast<int>(wParam));
            InvalidatePendingRenderRegions(true);
        }
        return 0;

    case WM_CHAR:
        if (auto focused = LockElement(m_focusedElement)) {
            if (auto tb = dynamic_cast<TextBox*>(focused.get())) {
                tb->OnCharInput(static_cast<wchar_t>(wParam));
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

    const D2D1_COLOR_F bgColor = D2D1::ColorF(0x1F / 255.0f, 0x1F / 255.0f, 0x1F / 255.0f, 1.0f);
    const D2D1_COLOR_F sceneClearColor = (m_transparentMode && !IsZoomed(m_hwnd))
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

    if (m_transparentMode && !IsZoomed(m_hwnd)) {
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

    // Restore: keep a 1px DWM inset for native snap/maximize animations.
    // Maximized: zero inset so the client fills the window and no white border shows.
    const bool maximized = IsZoomed(m_hwnd) != FALSE;
    const MARGINS margins = maximized ? MARGINS{ 0, 0, 0, 0 } : MARGINS{ 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);

    // 1. Force Native Windows 11 DWM Rounded Corners (DWMWA_WINDOW_CORNER_PREFERENCE = 33)
    DWM_WINDOW_CORNER_PREFERENCE preference = maximized ? DWMWCP_DONOTROUND : DWMWCP_ROUND;
    DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));

    // Match the app background on Win11's 1px window border.
    const COLORREF borderColor = RGB(0x1F, 0x1F, 0x1F);
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
        UIElement* menuHit = m_activeContextMenu->HitTestOverlay(fx, fy);
        if (menuHit) {
            menuHit->OnMouseDown(Point(fx, fy));
            m_activeContextMenu = nullptr;
            return true;
        } else {
            m_activeContextMenu->Hide();
            m_activeContextMenu = nullptr;
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
    auto focused = LockElement(m_focusedElement);
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
    if (target) {
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
        target->OnMouseRightClick(Point(fx, fy));

        UIElement* curr = target;
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu) {
                m_activeContextMenu = menu;
                m_activeContextMenu->ShowAt(fx, fy, winW, winH);
                SetFocusedElement(menu.get());
                break;
            }
            curr = curr->GetParent();
        }
    }
}

} // namespace CUI
