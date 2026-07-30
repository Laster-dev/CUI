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
#include <iostream>
#include <algorithm>
#include <chrono>

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

namespace CUI {

Window::Window() {}

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
    if (!element) return false;
    const char* cls = element->GetClassName();
    return std::strcmp(cls, "ListView") == 0
        || std::strcmp(cls, "ScrollViewer") == 0
        || std::strcmp(cls, "TabView") == 0;
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

void Window::InvalidateAnimatedRegions() {
    if (!m_hwnd || !m_rootElement) {
        return;
    }

    Rect dirtyRect;
    bool hasDirty = false;
    m_rootElement->CollectAnimationBounds(dirtyRect, hasDirty);

    if (!hasDirty) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }

    RECT rc = {
        static_cast<LONG>(std::floor(dirtyRect.x)) - 2,
        static_cast<LONG>(std::floor(dirtyRect.y)) - 2,
        static_cast<LONG>(std::ceil(dirtyRect.x + dirtyRect.width)) + 2,
        static_cast<LONG>(std::ceil(dirtyRect.y + dirtyRect.height)) + 2
    };
    InvalidateRect(m_hwnd, &rc, FALSE);
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
        UpdateWindow(m_hwnd);
    }
}

void Window::RunMessageLoop() {
    MSG msg = {};
    using clock = std::chrono::steady_clock;
    auto lastFrameTime = clock::now();
    constexpr auto targetFrame = std::chrono::milliseconds(16);
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

        const auto now = clock::now();
        const bool shouldProbeAnimation = hadMessage || animationActive;
        const bool frameDue = !animationActive || (now - lastFrameTime) >= targetFrame;
        bool animating = animationActive;
        bool didAnimationTick = false;
        if (shouldProbeAnimation && frameDue && m_rootElement) {
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

        if (didAnimationTick && animating) {
            InvalidateAnimatedRegions();
            UpdateWindow(m_hwnd);
            continue;
        }

        if (!hadMessage) {
            if (animationActive) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - lastFrameTime);
                DWORD waitMs = elapsed >= targetFrame
                    ? 0
                    : static_cast<DWORD>((targetFrame - elapsed).count());
                MsgWaitForMultipleObjectsEx(0, nullptr, waitMs, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            } else {
                WaitMessage();
            }
        }
    }
}

void Window::SetRootElement(std::shared_ptr<UIElement> root) {
    m_rootElement = root;
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
    switch (uMsg) {
    case WM_NCCALCSIZE:
        if (wParam == TRUE) {
            auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
            if (IsZoomed(m_hwnd)) {
                // Use the full maximized window rect as the client area so DWM does
                // not leave a 1px non-client strip (white border) around rcWork.
                // OnResize keeps an inner padding so content stays on-screen.
                params->rgrc[0] = params->rgrc[1];
            }
            return 0;
        }
        break;

    case WM_NCACTIVATE:
        // Pass to DefWindowProc with -1 margins to maintain native animation state
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

    case WM_SYSCOMMAND:
        // Handle window system commands (minimize, maximize, restore, close) with native animations
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
        ScreenToClient(m_hwnd, &pt);
        float fx = static_cast<float>(pt.x);
        float fy = static_cast<float>(pt.y);

        RECT rc;
        GetClientRect(m_hwnd, &rc);
        float winW = static_cast<float>(rc.right);
        float winH = static_cast<float>(rc.bottom);

        // 1. Resizing border handles (8-direction border resize) - only when NOT maximized
        if (!IsZoomed(m_hwnd)) {
            int borderThickness = 8;
            bool left = (pt.x < borderThickness);
            bool right = (pt.x >= rc.right - borderThickness);
            bool top = (pt.y < borderThickness);
            bool bottom = (pt.y >= rc.bottom - borderThickness);

            if (top && left) return HTTOPLEFT;
            if (top && right) return HTTOPRIGHT;
            if (bottom && left) return HTBOTTOMLEFT;
            if (bottom && right) return HTBOTTOMRIGHT;
            if (left) return HTLEFT;
            if (right) return HTRIGHT;
            if (top) return HTTOP;
            if (bottom) return HTBOTTOM;
        }

        // 2. Custom TitleBar Hit-Testing & Win11 Snap Layouts
        if (fy >= 0 && fy <= 40.0f) {
            // Close Button
            if (fx >= winW - 45.0f) {
                return HTCLOSE;
            }
            // Maximize / Restore Button -> Returns HTMAXBUTTON to trigger Win11 Snap Layouts
            if (fx >= winW - 90.0f && fx < winW - 45.0f) {
                return HTMAXBUTTON;
            }
            // Minimize Button
            if (fx >= winW - 135.0f && fx < winW - 90.0f) {
                return HTMINBUTTON;
            }

            // Drag window caption (only when clicking directly on empty TitleBar space, NOT child controls)
            if (fx < winW - 135.0f && m_rootElement) {
                UIElement* hit = m_rootElement->HitTest(fx, fy);
                if (auto titleBar = dynamic_cast<TitleBar*>(hit)) {
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

    case WM_NCLBUTTONDOWN:
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
        break;

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            if (auto focused = LockElement(m_focusedElement)) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        }
        return 0;

    case WM_SIZE:
        UpdateDwmChrome();
        OnResize(LOWORD(lParam), HIWORD(lParam));
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_MOUSEMOVE:
        if (OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONDBLCLK:
        OnLButtonDblClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONUP:
        if (OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_RBUTTONDOWN:
        OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(m_hwnd, nullptr, FALSE);
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
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (auto focused = LockElement(m_focusedElement)) {
            focused->OnKeyDown(static_cast<int>(wParam));
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_CHAR:
        if (auto focused = LockElement(m_focusedElement)) {
            if (auto tb = dynamic_cast<TextBox*>(focused.get())) {
                tb->OnCharInput(static_cast<wchar_t>(wParam));
                InvalidateRect(m_hwnd, nullptr, FALSE);
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
                    InvalidateRect(m_hwnd, nullptr, FALSE);
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
            InvalidateRect(m_hwnd, nullptr, FALSE);
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

    m_gfxContext.BeginDraw();

    const D2D1_COLOR_F bgColor = D2D1::ColorF(0x1F / 255.0f, 0x1F / 255.0f, 0x1F / 255.0f, 1.0f);
    if (m_transparentMode && !IsZoomed(m_hwnd)) {
        m_gfxContext.GetD2DContext()->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    } else {
        m_gfxContext.GetD2DContext()->Clear(bgColor);
    }

    if (m_rootElement) {
        m_rootElement->Render(m_gfxContext);
        m_rootElement->RenderOverlay(m_gfxContext);
    }

    if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
        m_activeContextMenu->OnRenderOverlay(m_gfxContext);
    }

    m_gfxContext.EndDraw();

    EndPaint(m_hwnd, &ps);
}

void Window::UpdateDwmChrome() {
    if (!m_hwnd) return;

    // Restore: keep a 1px DWM inset for native snap/maximize animations.
    // Maximized: zero inset so the client fills the window and no white border shows.
    const bool maximized = IsZoomed(m_hwnd) != FALSE;
    const MARGINS margins = maximized ? MARGINS{ 0, 0, 0, 0 } : MARGINS{ 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);

    // Match the app background on Win11's 1px window border.
    const COLORREF borderColor = RGB(0x1F, 0x1F, 0x1F);
    DwmSetWindowAttribute(m_hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

void Window::OnResize(UINT width, UINT height) {
    m_gfxContext.Resize(width, height);
    if (m_rootElement) {
        bool isMaximized = IsZoomed(m_hwnd);
        float pad = isMaximized ? 8.0f : 0.0f; // Add inner padding when maximized to prevent edge clipping

        Size avail(static_cast<float>(width) - pad * 2, static_cast<float>(height) - pad * 2);
        m_rootElement->Measure(avail);
        m_rootElement->Arrange(Rect(pad, pad, static_cast<float>(width) - pad * 2, static_cast<float>(height) - pad * 2));
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
