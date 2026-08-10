#include "MenuPopupWindow.h"
#include "Dpi.h"
#include "../controls/ContextMenu.h"
#include "../style/ThemeManager.h"
#include <windowsx.h>
#include <cmath>

namespace CUI {

namespace {
constexpr wchar_t kMenuPopupClass[] = L"CUI.MenuPopupWindow";
}

MenuPopupWindow::MenuPopupWindow() = default;

MenuPopupWindow::~MenuPopupWindow() {
    Hide();
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
    m_hwnd = nullptr;
}

bool MenuPopupWindow::IsMenuPopupHwnd(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return false;
    wchar_t cls[64] = {};
    if (GetClassNameW(hwnd, cls, 64) <= 0) return false;
    return wcscmp(cls, kMenuPopupClass) == 0;
}

void MenuPopupWindow::EnsureClass() {
    static bool registered = false;
    if (registered) return;
    registered = true;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kMenuPopupClass;
    RegisterClassExW(&wc);
}

bool MenuPopupWindow::EnsureWindow(HWND owner) {
    EnsureClass();
    if (m_hwnd && IsWindow(m_hwnd)) {
        m_owner = owner;
        return true;
    }

    m_owner = owner;
    m_hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
        kMenuPopupClass,
        L"",
        WS_POPUP,
        0, 0, 32, 32,
        owner,
        nullptr,
        GetModuleHandleW(nullptr),
        this);
    if (!m_hwnd) return false;

    m_dpiScale = GetDpiScaleForWindow(m_hwnd);
    m_gfx.SetRequirePerPixelAlpha(false);
    m_deviceReady = m_gfx.Initialize(m_hwnd);
    return m_deviceReady;
}

void MenuPopupWindow::ApplyRoundedRegion(int widthPx, int heightPx, float radiusDip) {
    if (!m_hwnd || !IsWindow(m_hwnd)) return;
    const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
    const int r = static_cast<int>(std::lround((std::max)(0.0f, radiusDip) * scale));
    if (r < 1) {
        SetWindowRgn(m_hwnd, nullptr, TRUE);
        return;
    }
    HRGN rgn = CreateRoundRectRgn(0, 0, widthPx + 1, heightPx + 1, r * 2, r * 2);
    if (rgn) {
        SetWindowRgn(m_hwnd, rgn, TRUE);
    }
}

bool MenuPopupWindow::Show(ContextMenu* menu, HWND owner, Point screenDipTopLeft, Size clientDipSize) {
    if (!menu || !owner) return false;
    if (!EnsureWindow(owner)) return false;

    m_menu = menu;
    m_hovered = nullptr;
    m_pressed = nullptr;
    m_dpiScale = GetDpiScaleForWindow(m_hwnd ? m_hwnd : owner);

    const int px = static_cast<int>(std::lround(screenDipTopLeft.x * m_dpiScale));
    const int py = static_cast<int>(std::lround(screenDipTopLeft.y * m_dpiScale));
    const int pw = (std::max)(1, static_cast<int>(std::lround(clientDipSize.width * m_dpiScale)));
    const int ph = (std::max)(1, static_cast<int>(std::lround(clientDipSize.height * m_dpiScale)));

    ApplyRoundedRegion(pw, ph, menu->GetCornerRadius());
    SetWindowPos(m_hwnd, HWND_TOPMOST, px, py, pw, ph,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    m_gfx.Resize(static_cast<UINT>(pw), static_cast<UINT>(ph));
    Invalidate();
    return true;
}

void MenuPopupWindow::Hide() {
    m_hovered = nullptr;
    m_pressed = nullptr;
    m_menu = nullptr;
    if (m_hwnd && IsWindow(m_hwnd)) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void MenuPopupWindow::Invalidate() {
    if (m_hwnd && IsWindow(m_hwnd)) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

Point MenuPopupWindow::ClientDipToScreenDip(float clientX, float clientY) const {
    if (!m_hwnd) return Point(clientX, clientY);
    const float s = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
    POINT pt{
        static_cast<LONG>(std::lround(clientX * s)),
        static_cast<LONG>(std::lround(clientY * s))
    };
    ClientToScreen(m_hwnd, &pt);
    return Point(static_cast<float>(pt.x) / s, static_cast<float>(pt.y) / s);
}

Point MenuPopupWindow::ClientPhysicalToDip(int x, int y) const {
    float lx = 0.0f;
    float ly = 0.0f;
    ClientPhysicalToLogical(m_hwnd, x, y, lx, ly);
    return Point(lx, ly);
}

void MenuPopupWindow::Paint() {
    if (!m_menu || !m_deviceReady) return;

    m_gfx.BeginDraw();
    D2D1_COLOR_F clear = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
    clear.a = 1.0f;
    if (auto* ctx = m_gfx.GetD2DContext()) {
        ctx->Clear(clear);
    }
    m_menu->RenderPopup(m_gfx);
    m_gfx.EndDraw();
}

void MenuPopupWindow::HandleMouseMove(int x, int y) {
    if (!m_menu) return;
    Point pt = ClientPhysicalToDip(x, y);

    TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, m_hwnd, 0 };
    TrackMouseEvent(&tme);

    if (m_pressed) {
        m_pressed->OnMouseMove(pt);
        Invalidate();
        return;
    }
    UIElement* hit = m_menu->HitTestPopup(pt.x, pt.y);
    if (hit != m_hovered) {
        if (m_hovered) m_hovered->OnMouseLeave();
        m_hovered = hit;
        if (m_hovered) m_hovered->OnMouseEnter();
    }
    if (m_hovered) m_hovered->OnMouseMove(pt);
    Invalidate();
}

void MenuPopupWindow::HandleMouseButton(UINT msg, int x, int y) {
    // Capture mouse-up even if Hide cleared m_menu mid-click (owner deactivate).
    ContextMenu* menu = m_menu;
    Point pt = ClientPhysicalToDip(x, y);

    if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN) {
        if (!menu) return;
        UIElement* hit = menu->HitTestPopup(pt.x, pt.y);
        m_pressed = hit;
        if (m_pressed) {
            SetCapture(m_hwnd);
            m_pressed->OnMouseDown(pt);
        } else {
            menu->DismissHierarchy();
        }
        Invalidate();
        return;
    }

    if (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP) {
        UIElement* pressed = m_pressed;
        m_pressed = nullptr;
        if (GetCapture() == m_hwnd) {
            ReleaseCapture();
        }

        // Prefer the pressed element; fall back to current hit so a brief
        // deactivate/Hide cannot swallow the command.
        UIElement* target = pressed;
        if ((!target || !menu) && menu) {
            target = menu->HitTestPopup(pt.x, pt.y);
        }
        if (!target && pressed) {
            target = pressed;
        }

        if (auto* item = dynamic_cast<MenuItem*>(target)) {
            if (item->IsEnabled() && !item->HasSubMenu() && !item->IsSeparator()) {
                // Direct invoke — do not require IsPressed (MOUSELEAVE may clear it).
                item->ExecuteCommand();
            } else if (pressed) {
                pressed->OnMouseUp(pt);
            }
        } else if (pressed) {
            pressed->OnMouseUp(pt);
        }
        Invalidate();
    }
}

LRESULT CALLBACK MenuPopupWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MenuPopupWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<MenuPopupWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<MenuPopupWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        BeginPaint(hwnd, &ps);
        self->Paint();
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_MOUSEMOVE:
        self->HandleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        self->HandleMouseButton(msg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSELEAVE:
        // Do not clear pressed here — OnMouseLeave clears IsPressed and used to
        // race with click invoke when the cursor briefly leaves during capture.
        if (self->m_hovered && self->m_hovered != self->m_pressed) {
            self->m_hovered->OnMouseLeave();
            self->m_hovered = nullptr;
            self->Invalidate();
        } else if (self->m_hovered && !self->m_pressed) {
            self->m_hovered->OnMouseLeave();
            self->m_hovered = nullptr;
            self->Invalidate();
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (self->m_menu) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
            self->m_menu->OnMouseWheel(delta);
            self->Invalidate();
        }
        return 0;
    case WM_DESTROY:
        self->m_hwnd = nullptr;
        self->m_deviceReady = false;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace CUI
