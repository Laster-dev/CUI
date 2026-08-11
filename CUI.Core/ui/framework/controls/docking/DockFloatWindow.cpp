#include "DockFloatWindow.h"
#include "DockManager.h"
#include "../../window/Dpi.h"
#include "../../style/ThemeManager.h"
#include "../../controls/UIElement.h"
#include <windowsx.h>
#include <cmath>

#ifndef WS_EX_NOREDIRECTIONBITMAP
#define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif

namespace CUI {

namespace {
constexpr wchar_t kDockFloatClass[] = L"CUI.DockFloatWindow";

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return L"";
    }
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (n <= 1) {
        return L"";
    }
    std::wstring out(static_cast<size_t>(n - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, out.data(), n);
    return out;
}
} // namespace

DockFloatWindow::DockFloatWindow() = default;

DockFloatWindow::~DockFloatWindow() {
    Destroy();
}

bool DockFloatWindow::IsDockFloatHwnd(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }
    wchar_t cls[64] = {};
    if (GetClassNameW(hwnd, cls, 64) <= 0) {
        return false;
    }
    return wcscmp(cls, kDockFloatClass) == 0;
}

DockFloatWindow* DockFloatWindow::FromHwnd(HWND hwnd) {
    if (!IsDockFloatHwnd(hwnd)) {
        return nullptr;
    }
    return reinterpret_cast<DockFloatWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

void DockFloatWindow::EnsureClass() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kDockFloatClass;
    RegisterClassExW(&wc);
}

bool DockFloatWindow::EnsureWindow(HWND owner) {
    EnsureClass();
    if (m_hwnd && IsWindow(m_hwnd)) {
        m_owner = owner;
        return true;
    }
    m_owner = owner;
    m_hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOREDIRECTIONBITMAP,
        kDockFloatClass,
        m_title.c_str(),
        WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU,
        0, 0, 400, 300,
        owner,
        nullptr,
        GetModuleHandleW(nullptr),
        this);
    if (!m_hwnd) {
        return false;
    }
    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    m_dpiScale = GetDpiScaleForWindow(m_hwnd);
    m_deviceReady = m_gfx.Initialize(m_hwnd);
    return m_deviceReady;
}

void DockFloatWindow::SetTitleFromPane() {
    if (m_manager) {
        if (const DockPaneData* p = m_manager->GetPane(m_paneIndex)) {
            m_title = Utf8ToWide(p->title);
            if (m_hwnd) {
                SetWindowTextW(m_hwnd, m_title.c_str());
            }
        }
    }
}

bool DockFloatWindow::Show(DockManager* manager,
                           int paneIndex,
                           HWND owner,
                           Point screenDipTopLeft,
                           Size clientDipSize) {
    if (!manager || paneIndex < 0 || !owner) {
        return false;
    }
    const DockPaneData* pane = manager->GetPane(paneIndex);
    if (!pane) {
        return false;
    }
    if (!EnsureWindow(owner)) {
        return false;
    }
    m_manager = manager;
    m_paneIndex = paneIndex;
    m_content = pane->content;
    if (m_content) {
        // Detach from main dock tree so we paint it in this HWND.
        manager->RemoveChildQuiet(m_content);
        m_content->SetVisibility(Visibility::Visible);
    }
    SetTitleFromPane();

    m_dpiScale = GetDpiScaleForWindow(m_hwnd);
    const int px = static_cast<int>(std::lround(screenDipTopLeft.x * m_dpiScale));
    const int py = static_cast<int>(std::lround(screenDipTopLeft.y * m_dpiScale));
    const int pw = (std::max)(160, static_cast<int>(std::lround(clientDipSize.width * m_dpiScale)));
    const int ph = (std::max)(120, static_cast<int>(std::lround(clientDipSize.height * m_dpiScale)));

    RECT rc{ 0, 0, pw, ph };
    AdjustWindowRectEx(&rc, GetWindowLongW(m_hwnd, GWL_STYLE), FALSE, GetWindowLongW(m_hwnd, GWL_EXSTYLE));
    SetWindowPos(m_hwnd, HWND_TOP, px, py, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
    Relayout();
    InvalidateRect(m_hwnd, nullptr, FALSE);
    return true;
}

void DockFloatWindow::Destroy() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
    if (m_manager && m_content) {
        m_manager->AddChildQuiet(m_content);
        m_content.reset();
    }
    m_manager = nullptr;
    m_paneIndex = -1;
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void DockFloatWindow::RemapPaneIndexAfterClose(int closedIndex) {
    if (m_paneIndex > closedIndex) {
        --m_paneIndex;
    } else if (m_paneIndex == closedIndex) {
        m_paneIndex = -1;
    }
}

Point DockFloatWindow::ClientPhysicalToDip(int x, int y) const {
    return Point(static_cast<float>(x) / m_dpiScale, static_cast<float>(y) / m_dpiScale);
}

void DockFloatWindow::Relayout() {
    if (!m_hwnd || !m_content) {
        return;
    }
    RECT crc{};
    GetClientRect(m_hwnd, &crc);
    const float cw = static_cast<float>(crc.right) / m_dpiScale;
    const float ch = static_cast<float>(crc.bottom) / m_dpiScale;
    const Rect content(0.0f, m_headerH, cw, (std::max)(0.0f, ch - m_headerH));
    m_content->SetVisibility(Visibility::Visible);
    m_content->Measure(Size(content.width, content.height));
    m_content->Arrange(content);
}

void DockFloatWindow::Paint() {
    if (!m_deviceReady || !m_hwnd) {
        return;
    }
    m_dpiScale = GetDpiScaleForWindow(m_hwnd);
    RECT crc{};
    GetClientRect(m_hwnd, &crc);
    const float cw = static_cast<float>(crc.right) / m_dpiScale;
    const float ch = static_cast<float>(crc.bottom) / m_dpiScale;

    m_gfx.BeginDraw();
    const auto& tokens = ThemeManager::Instance().GetTokens();
    m_gfx.ClearRect(Rect(0.0f, 0.0f, cw, ch), tokens.windowBackground);

    // Self-drawn header
    const Rect header(0.0f, 0.0f, cw, m_headerH);
    m_gfx.FillRect(header, tokens.cardBackground);
    m_gfx.DrawLine(Point(0, m_headerH - 0.5f), Point(cw, m_headerH - 0.5f), tokens.cardBorder, 1.0f);
    std::string title = "Tool";
    if (m_manager) {
        if (const auto* p = m_manager->GetPane(m_paneIndex)) {
            title = p->title;
        }
    }
    m_gfx.DrawText(
        title,
        Rect(10.0f, 0.0f, cw - 20.0f, m_headerH),
        tokens.textPrimary,
        "Segoe UI",
        12.0f,
        DWRITE_TEXT_ALIGNMENT_LEADING,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        true);

    Relayout();
    if (m_content) {
        m_content->Render(m_gfx);
    }
    m_gfx.EndDraw();
}

void DockFloatWindow::HandleMouse(UINT msg, int x, int y) {
    const Point pt = ClientPhysicalToDip(x, y);
    if (!m_content) {
        return;
    }
    switch (msg) {
    case WM_LBUTTONDOWN:
        SetCapture(m_hwnd);
        m_pressed = m_content->HitTest(pt.x, pt.y);
        if (m_pressed) {
            m_pressed->OnMouseDown(pt);
        }
        break;
    case WM_LBUTTONUP:
        if (m_pressed) {
            m_pressed->OnMouseUp(pt);
            m_pressed = nullptr;
        }
        ReleaseCapture();
        break;
    case WM_MOUSEMOVE: {
        UIElement* hit = m_content->HitTest(pt.x, pt.y);
        if (hit != m_hovered) {
            if (m_hovered) {
                m_hovered->OnMouseLeave();
            }
            m_hovered = hit;
            if (m_hovered) {
                m_hovered->OnMouseEnter();
            }
        }
        if (m_pressed) {
            m_pressed->OnMouseMove(pt);
        } else if (m_hovered) {
            m_hovered->OnMouseMove(pt);
        }
        break;
    }
    default:
        break;
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

LRESULT CALLBACK DockFloatWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DockFloatWindow* self = FromHwnd(hwnd);
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<DockFloatWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self) {
            self->m_hwnd = hwnd;
        }
        return TRUE;
    }
    if (!self) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        BeginPaint(hwnd, &ps);
        self->Paint();
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SIZE:
        self->Relayout();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
        self->HandleMouse(msg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_CLOSE:
        if (self->m_onClose) {
            // Restore pane into dock before destroying.
            const int pane = self->m_paneIndex;
            auto content = self->m_content;
            self->m_content.reset();
            if (self->m_manager && content) {
                // Manager NotifyFloatClosed will dock; re-attach content there.
                self->m_manager->AddChildQuiet(content);
            }
            auto cb = self->m_onClose;
            cb(self);
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        self->m_hwnd = nullptr;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace CUI
