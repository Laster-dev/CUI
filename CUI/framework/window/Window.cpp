#include "Window.h"
#include "../controls/TextBox.h"
#include "../controls/ContextMenu.h"
#include <windowsx.h>
#include <imm.h>
#include <iostream>

#pragma comment(lib, "imm32.lib")

namespace CUI {

Window::Window() {}

Window::~Window() {
    if (m_hwnd) {
        KillTimer(m_hwnd, 1);
        DestroyWindow(m_hwnd);
    }
}

bool Window::Create(const std::string& title, int width, int height) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // Direct2D double buffering
    wc.lpszClassName = L"CUI_WindowClass";

    RegisterClassEx(&wc);

    std::wstring wTitle(title.begin(), title.end());

    m_hwnd = CreateWindowEx(
        0,
        L"CUI_WindowClass",
        wTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        hInstance,
        this
    );

    if (!m_hwnd) return false;

    if (!m_gfxContext.Initialize(m_hwnd)) {
        return false;
    }

    SetTimer(m_hwnd, 1, 500, nullptr); // 500ms Cursor Blink Timer

    return true;
}

void Window::Show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }
}

void Window::RunMessageLoop() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Window::SetRootElement(std::shared_ptr<UIElement> root) {
    m_rootElement = root;
    if (m_hwnd) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        OnResize(rc.right - rc.left, rc.bottom - rc.top);
    }
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
    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_TIMER:
        if (m_focusedElement) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_SIZE:
        OnResize(LOWORD(lParam), HIWORD(lParam));
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_MOUSEMOVE:
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDBLCLK:
        OnLButtonDblClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONUP:
        OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_RBUTTONDOWN:
        OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_MOUSEWHEEL: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(m_hwnd, &pt);
        float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        UIElement* target = m_rootElement ? m_rootElement->HitTest(static_cast<float>(pt.x), static_cast<float>(pt.y)) : nullptr;
        if (target) {
            target->OnMouseWheel(delta);
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (m_focusedElement) {
            m_focusedElement->OnKeyDown(static_cast<int>(wParam));
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_CHAR:
        if (m_focusedElement) {
            if (auto tb = dynamic_cast<TextBox*>(m_focusedElement)) {
                tb->OnCharInput(static_cast<wchar_t>(wParam));
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        }
        return 0;

    case WM_IME_COMPOSITION:
        if (m_focusedElement) {
            if (auto tb = dynamic_cast<TextBox*>(m_focusedElement)) {
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
                            tb->SetCompositionString(L"");
                            for (wchar_t ch : resultStr) {
                                tb->OnCharInput(ch);
                            }
                        }
                    }
                    ImmReleaseContext(m_hwnd, hIMC);
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
            }
        }
        break;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && m_hoveredElement) {
            HCURSOR hCur = m_hoveredElement->GetCursor();
            if (hCur) {
                SetCursor(hCur);
                return TRUE;
            }
        }
        break;

    case WM_MOUSELEAVE:
        if (m_hoveredElement) {
            m_hoveredElement->OnMouseLeave();
            m_hoveredElement = nullptr;
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

    m_gfxContext.BeginDraw();

    // Clear background with VS Code dark color #1F1F1F
    m_gfxContext.GetD2DContext()->Clear(D2D1::ColorF(0x1F / 255.0f, 0x1F / 255.0f, 0x1F / 255.0f, 1.0f));

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

void Window::OnResize(UINT width, UINT height) {
    m_gfxContext.Resize(width, height);
    if (m_rootElement) {
        Size avail(static_cast<float>(width), static_cast<float>(height));
        m_rootElement->Measure(avail);
        m_rootElement->Arrange(Rect(0, 0, static_cast<float>(width), static_cast<float>(height)));
    }
}

void Window::OnMouseMove(int x, int y) {
    if (!m_trackingMouse) {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hwnd, 0 };
        TrackMouseEvent(&tme);
        m_trackingMouse = true;
    }

    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
        UIElement* itemHover = m_activeContextMenu->HitTestOverlay(fx, fy);
        if (itemHover != m_hoveredElement) {
            if (m_hoveredElement) m_hoveredElement->OnMouseLeave();
            m_hoveredElement = itemHover;
            if (m_hoveredElement) m_hoveredElement->OnMouseEnter();
        }
        if (m_hoveredElement) m_hoveredElement->OnMouseMove(Point(fx, fy));
        return;
    }

    UIElement* newHover = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
    if (newHover != m_hoveredElement) {
        if (m_hoveredElement) {
            m_hoveredElement->OnMouseLeave();
        }
        m_hoveredElement = newHover;
        if (m_hoveredElement) {
            m_hoveredElement->OnMouseEnter();
        }
    }

    if (m_hoveredElement) {
        m_hoveredElement->OnMouseMove(Point(fx, fy));
    }
}

void Window::OnLButtonDown(int x, int y) {
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    if (m_activeContextMenu && m_activeContextMenu->IsOpen()) {
        UIElement* menuHit = m_activeContextMenu->HitTestOverlay(fx, fy);
        if (menuHit) {
            menuHit->OnMouseDown(Point(fx, fy));
            m_activeContextMenu = nullptr;
            return;
        } else {
            m_activeContextMenu->Hide();
            m_activeContextMenu = nullptr;
        }
    }

    UIElement* target = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
    if (m_focusedElement && m_focusedElement != target) {
        m_focusedElement->OnBlur();
    }
    m_focusedElement = target;
    if (m_focusedElement) {
        m_focusedElement->OnFocus();
    }

    if (target) {
        m_pressedElement = target;
        m_pressedElement->OnMouseDown(Point(fx, fy));
    }
}

void Window::OnLButtonDblClick(int x, int y) {
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    UIElement* target = m_rootElement ? m_rootElement->HitTest(fx, fy) : nullptr;
    if (target) {
        target->OnMouseDblClick(Point(fx, fy));
    }
}

void Window::OnLButtonUp(int x, int y) {
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    if (m_pressedElement) {
        m_pressedElement->OnMouseUp(Point(fx, fy));
        m_pressedElement = nullptr;
    }
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

        // Find context menu on target or ancestor
        UIElement* curr = target;
        while (curr) {
            auto menu = curr->GetContextMenu();
            if (menu) {
                m_activeContextMenu = menu;
                m_activeContextMenu->ShowAt(fx, fy, winW, winH);
                m_focusedElement = menu.get();
                break;
            }
            curr = curr->GetParent();
        }
    }
}

} // namespace CUI
