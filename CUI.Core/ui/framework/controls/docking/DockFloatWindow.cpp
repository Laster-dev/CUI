#include "DockFloatWindow.h"
#include "DockManager.h"
#include "../../window/Dpi.h"
#include "../../style/ThemeManager.h"
#include "../../controls/UIElement.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <vector>

#pragma comment(lib, "dwmapi.lib")

#ifndef WS_EX_NOREDIRECTIONBITMAP
#define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

namespace CUI {

namespace {
constexpr wchar_t kDockFloatClass[] = L"CUI.DockFloatWindow";
constexpr UINT WM_CUI_FLOAT_REDOCK = WM_APP + 77;
constexpr float kCaptionBtnW = 46.0f;
constexpr float kIconSize = 18.0f;
constexpr float kResizeBorderDip = 8.0f;

std::vector<DockFloatWindow*> g_liveFloats;

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

std::string DefaultIconText(const std::string& title) {
    for (unsigned char ch : title) {
        if (std::isalnum(ch)) {
            return std::string(1, static_cast<char>(std::toupper(ch)));
        }
    }
    return "C";
}
} // namespace

DockFloatWindow::DockFloatWindow() {
    g_liveFloats.push_back(this);
}

DockFloatWindow::~DockFloatWindow() {
    g_liveFloats.erase(std::remove(g_liveFloats.begin(), g_liveFloats.end(), this), g_liveFloats.end());
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
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = kDockFloatClass;
    RegisterClassExW(&wc);
}

void DockFloatWindow::ApplyCuiChrome() {
    if (!m_hwnd) {
        return;
    }
    const bool maximized = IsZoomed(m_hwnd) != FALSE;
    const MARGINS margins = maximized ? MARGINS{ 0, 0, 0, 0 } : MARGINS{ 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);

    DWM_WINDOW_CORNER_PREFERENCE preference = maximized ? DWMWCP_DONOTROUND : DWMWCP_ROUND;
    DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));

    const D2D1_COLOR_F border = ThemeManager::Instance().GetTokens().windowBackground;
    const COLORREF borderColor = RGB(
        static_cast<int>(border.r * 255.0f),
        static_cast<int>(border.g * 255.0f),
        static_cast<int>(border.b * 255.0f));
    DwmSetWindowAttribute(m_hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

bool DockFloatWindow::EnsureWindow(HWND owner) {
    EnsureClass();
    if (m_hwnd && IsWindow(m_hwnd)) {
        m_owner = owner;
        return true;
    }
    m_owner = owner;
    m_hwnd = CreateWindowExW(
        WS_EX_NOREDIRECTIONBITMAP | WS_EX_APPWINDOW,
        kDockFloatClass,
        m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,
        owner,
        nullptr,
        GetModuleHandleW(nullptr),
        this);
    if (!m_hwnd) {
        return false;
    }
    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    m_dpiScale = GetDpiScaleForWindow(m_hwnd);
    ApplyCuiChrome();
    m_deviceReady = m_gfx.Initialize(m_hwnd);
    if (m_deviceReady) {
        RECT crc{};
        GetClientRect(m_hwnd, &crc);
        m_lastPxW = static_cast<UINT>((std::max)(1L, crc.right));
        m_lastPxH = static_cast<UINT>((std::max)(1L, crc.bottom));
    }
    return m_deviceReady;
}

void DockFloatWindow::SetTitleFromPane() {
    if (m_manager) {
        if (const DockPaneData* p = m_manager->GetPane(m_paneIndex)) {
            m_titleUtf8 = p->title;
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
                           Size windowDipSize) {
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
        // Stay parented under DockManager so AnimationManager::IsInLiveTree succeeds.
        m_content->SetPresentsOnOwnerWindow(false);
        m_content->SetVisibility(Visibility::Visible);
        m_content->SetAlign(Alignment::Stretch);
        m_content->SetWidth(-1.0f);
        m_content->SetHeight(-1.0f);
        if (m_content->HasSelfAnimation()) {
            m_content->RequestAnimationTicks();
        }
    }
    SetTitleFromPane();

    m_dpiScale = GetDpiScaleForWindow(m_hwnd);
    const int px = static_cast<int>(std::lround(screenDipTopLeft.x * m_dpiScale));
    const int py = static_cast<int>(std::lround(screenDipTopLeft.y * m_dpiScale));
    const int pw = (std::max)(240, static_cast<int>(std::lround(windowDipSize.width * m_dpiScale)));
    const int ph = (std::max)(180, static_cast<int>(std::lround(windowDipSize.height * m_dpiScale)));

    SetWindowPos(m_hwnd, HWND_TOP, px, py, pw, ph, SWP_NOACTIVATE);
    HandleClientSize();
    ApplyCuiChrome();
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    Relayout();
    InvalidateRect(m_hwnd, nullptr, FALSE);
    return true;
}

void DockFloatWindow::DetachContent() {
    if (m_content) {
        m_content->SetPresentsOnOwnerWindow(true);
        if (m_manager) {
            bool found = false;
            for (const auto& ch : m_manager->GetChildren()) {
                if (ch.get() == m_content.get()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                m_manager->AddChildQuiet(m_content);
            }
        }
        m_content.reset();
    }
}

void DockFloatWindow::Destroy() {
    if (m_destroying) {
        return;
    }
    m_destroying = true;
    DetachContent();
    m_manager = nullptr;
    m_paneIndex = -1;
    HWND hwnd = m_hwnd;
    m_hwnd = nullptr;
    if (hwnd && IsWindow(hwnd)) {
        DestroyWindow(hwnd);
    }
    m_destroying = false;
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

Rect DockFloatWindow::TitleBarRect() const {
    RECT crc{};
    if (m_hwnd) {
        GetClientRect(m_hwnd, &crc);
    }
    const float cw = static_cast<float>(crc.right) / m_dpiScale;
    return Rect(0.0f, 0.0f, cw, m_titleH);
}

Rect DockFloatWindow::CaptionButtonRect(int index) const {
    const Rect bar = TitleBarRect();
    const float right = bar.x + bar.width;
    return Rect(right - kCaptionBtnW * static_cast<float>(3 - index), bar.y, kCaptionBtnW, bar.height);
}

int DockFloatWindow::HitCaptionButton(float x, float y) const {
    for (int i = 0; i < 3; ++i) {
        if (CaptionButtonRect(i).Contains(x, y)) {
            return i;
        }
    }
    return -1;
}

LRESULT DockFloatWindow::HitTest(int screenX, int screenY) const {
    POINT pt{ screenX, screenY };
    ScreenToClient(m_hwnd, &pt);
    const Point dip = ClientPhysicalToDip(pt.x, pt.y);

    RECT rc{};
    GetClientRect(m_hwnd, &rc);
    const float scale = (m_dpiScale > 0.001f) ? m_dpiScale : 1.0f;
    const int border = (std::max)(1, static_cast<int>(std::lround(kResizeBorderDip * scale)));

    const int btn = HitCaptionButton(dip.x, dip.y);
    if (btn == 0) {
        return HTMINBUTTON;
    }
    if (btn == 1) {
        return HTMAXBUTTON;
    }
    if (btn == 2) {
        return HTCLOSE;
    }

    if (!IsZoomed(m_hwnd)) {
        const bool left = (pt.x < border);
        const bool right = (pt.x >= rc.right - border);
        const bool top = (pt.y < border);
        const bool bottom = (pt.y >= rc.bottom - border);
        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;
    }

    if (TitleBarRect().Contains(dip.x, dip.y)) {
        return HTCAPTION;
    }
    return HTCLIENT;
}

void DockFloatWindow::SetCaptionHover(int region) {
    if (m_captionHover == region) {
        return;
    }
    m_captionHover = region;
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

Point DockFloatWindow::CursorScreenDip() const {
    POINT pt{};
    GetCursorPos(&pt);
    HWND scaleHwnd = m_owner ? m_owner : m_hwnd;
    const float scale = GetDpiScaleForWindow(scaleHwnd);
    return Point(static_cast<float>(pt.x) / scale, static_cast<float>(pt.y) / scale);
}

void DockFloatWindow::BeginRedockTracking() {
    if (m_redocking || !m_manager || m_paneIndex < 0) {
        return;
    }
    m_redocking = true;
    m_manager->BeginFloatRedock(m_paneIndex);
}

void DockFloatWindow::UpdateRedockTracking() {
    if (!m_redocking || !m_manager) {
        return;
    }
    m_manager->UpdateFloatRedock(CursorScreenDip());
}

void DockFloatWindow::FinishRedockTracking() {
    if (!m_redocking) {
        return;
    }
    m_redocking = false;
    PostMessageW(m_hwnd, WM_CUI_FLOAT_REDOCK, 0, 0);
}

void DockFloatWindow::PresentAll() {
    for (DockFloatWindow* wnd : g_liveFloats) {
        if (wnd && wnd->m_hwnd && wnd->m_deviceReady && wnd->m_content && !IsIconic(wnd->m_hwnd)) {
            wnd->Paint();
        }
    }
}

void DockFloatWindow::TrackClientMouse() {
    if (m_trackingMouse || !m_hwnd) {
        return;
    }
    TRACKMOUSEEVENT tme{};
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = m_hwnd;
    if (TrackMouseEvent(&tme)) {
        m_trackingMouse = true;
    }
}

void DockFloatWindow::HandleWheel(float delta, int x, int y) {
    const Point pt = ClientPhysicalToDip(x, y);
    UIElement* hit = m_content ? m_content->HitTest(pt.x, pt.y) : nullptr;
    if (hit) {
        hit->OnMouseWheel(delta);
    } else if (m_content) {
        m_content->OnMouseWheel(delta);
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DockFloatWindow::HandleClientSize() {
    if (!m_hwnd) {
        return;
    }
    m_dpiScale = GetDpiScaleForWindow(m_hwnd);
    RECT crc{};
    GetClientRect(m_hwnd, &crc);
    const UINT w = static_cast<UINT>((std::max)(1L, crc.right));
    const UINT h = static_cast<UINT>((std::max)(1L, crc.bottom));
    if (m_deviceReady && (w != m_lastPxW || h != m_lastPxH)) {
        m_gfx.Resize(w, h);
        m_lastPxW = w;
        m_lastPxH = h;
        Relayout();
    } else if (m_lastPxW == 0) {
        m_lastPxW = w;
        m_lastPxH = h;
        Relayout();
    }
}

void DockFloatWindow::Relayout() {
    if (!m_hwnd || !m_content) {
        return;
    }
    RECT crc{};
    GetClientRect(m_hwnd, &crc);
    const float cw = static_cast<float>(crc.right) / m_dpiScale;
    const float ch = static_cast<float>(crc.bottom) / m_dpiScale;
    const float bodyH = (std::max)(0.0f, ch - m_titleH);
    const Rect content(0.0f, m_titleH, cw, bodyH);
    m_content->SetVisibility(Visibility::Visible);
    m_content->Measure(Size(content.width, content.height));
    m_content->Arrange(content);
}

void DockFloatWindow::Paint() {
    if (!m_deviceReady || !m_hwnd) {
        return;
    }
    RECT crc{};
    GetClientRect(m_hwnd, &crc);
    const float cw = static_cast<float>(crc.right) / m_dpiScale;
    const float ch = static_cast<float>(crc.bottom) / m_dpiScale;

    m_gfx.BeginDraw();
    const auto& tokens = ThemeManager::Instance().GetTokens();
    m_gfx.ClearRect(Rect(0.0f, 0.0f, cw, ch), tokens.windowBackground);

    const Rect header(0.0f, 0.0f, cw, m_titleH);
    m_gfx.FillRect(header, tokens.paneBackground);

    const Rect iconRect(10.0f, (m_titleH - kIconSize) * 0.5f, kIconSize, kIconSize);
    m_gfx.FillRoundedRect(iconRect, 4.0f, tokens.accentColor);
    m_gfx.DrawText(
        DefaultIconText(m_titleUtf8),
        iconRect,
        tokens.accentForeground,
        "微软雅黑",
        11.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
        DWRITE_FONT_WEIGHT_BOLD);

    const Rect minRect = CaptionButtonRect(0);
    const Rect maxRect = CaptionButtonRect(1);
    const Rect closeRect = CaptionButtonRect(2);
    const bool light = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;
    const float hoverPeak = light ? 0.08f : 0.14f;

    if (m_captionHover == 0) {
        m_gfx.FillRect(minRect, D2D1::ColorF(
            tokens.textPrimary.r, tokens.textPrimary.g, tokens.textPrimary.b, hoverPeak));
    }
    if (m_captionHover == 1) {
        m_gfx.FillRect(maxRect, D2D1::ColorF(
            tokens.textPrimary.r, tokens.textPrimary.g, tokens.textPrimary.b, hoverPeak));
    }
    if (m_captionHover == 2) {
        D2D1_COLOR_F closeBg = tokens.dangerColor;
        closeBg.a = 0.92f;
        m_gfx.FillRect(closeRect, closeBg);
    }

    const float titleLeft = iconRect.x + iconRect.width + 10.0f;
    const float titleRight = minRect.x - 8.0f;
    if (titleRight > titleLeft) {
        m_gfx.DrawText(
            m_titleUtf8,
            Rect(titleLeft, 0.0f, titleRight - titleLeft, m_titleH),
            tokens.textPrimary,
            "微软雅黑",
            13.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            true);
    }

    const float iconCy = m_titleH * 0.5f;
    m_gfx.DrawSmoothLine(
        Point(minRect.x + 18.0f, iconCy),
        Point(minRect.x + 28.0f, iconCy),
        tokens.textPrimary,
        1.4f);

    const bool maximized = IsZoomed(m_hwnd) != FALSE;
    if (maximized) {
        m_gfx.DrawRect(Rect(maxRect.x + 20.0f, iconCy - 4.0f, 8.0f, 8.0f), tokens.textPrimary, 1.0f);
        m_gfx.DrawRect(Rect(maxRect.x + 18.0f, iconCy - 6.0f, 8.0f, 8.0f), tokens.textPrimary, 1.0f);
    } else {
        m_gfx.DrawRect(Rect(maxRect.x + 18.0f, iconCy - 5.0f, 10.0f, 10.0f), tokens.textPrimary, 1.0f);
    }

    D2D1_COLOR_F closeIcon = (m_captionHover == 2) ? tokens.accentForeground : tokens.textPrimary;
    const float closeCx = closeRect.x + closeRect.width * 0.5f;
    m_gfx.DrawSmoothLine(Point(closeCx - 5.0f, iconCy - 5.0f), Point(closeCx + 5.0f, iconCy + 5.0f), closeIcon, 1.5f);
    m_gfx.DrawSmoothLine(Point(closeCx + 5.0f, iconCy - 5.0f), Point(closeCx - 5.0f, iconCy + 5.0f), closeIcon, 1.5f);

    m_gfx.DrawLine(
        Point(0.0f, m_titleH - 1.0f),
        Point(cw, m_titleH - 1.0f),
        tokens.cardBorder,
        1.0f);

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
        TrackClientMouse();
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
    case WM_MOUSELEAVE:
        m_trackingMouse = false;
        if (m_hovered) {
            m_hovered->OnMouseLeave();
            m_hovered = nullptr;
        }
        break;
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
    case WM_NCCALCSIZE:
        // Minimized owned windows become a tiny bottom-left icon if we keep
        // swallowing non-client. Let DefWindowProc size the iconic window, and
        // WS_EX_APPWINDOW puts a taskbar button so it can be restored.
        if (IsIconic(hwnd)) {
            break;
        }
        return 0;
    case WM_NCACTIVATE:
        if (IsIconic(hwnd)) {
            break;
        }
        return TRUE;
    case WM_NCPAINT:
        if (IsIconic(hwnd)) {
            break;
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_NCHITTEST:
        return self->HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    case WM_NCMOUSEMOVE: {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);
        const Point dip = self->ClientPhysicalToDip(pt.x, pt.y);
        self->SetCaptionHover(self->HitCaptionButton(dip.x, dip.y));
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE | TME_NONCLIENT;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_NCMOUSELEAVE:
        self->SetCaptionHover(-1);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        BeginPaint(hwnd, &ps);
        self->Paint();
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            return 0;
        }
        self->ApplyCuiChrome();
        self->HandleClientSize();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_DPICHANGED: {
        self->m_dpiScale = static_cast<float>(LOWORD(wParam)) / 96.0f;
        const RECT* rc = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hwnd, nullptr, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        const float scale = (self->m_dpiScale > 0.001f) ? self->m_dpiScale : 1.0f;
        mmi->ptMinTrackSize.x = static_cast<LONG>(240.0f * scale);
        mmi->ptMinTrackSize.y = static_cast<LONG>(160.0f * scale);
        return 0;
    }
    case WM_MOVING:
        self->BeginRedockTracking();
        self->UpdateRedockTracking();
        break;
    case WM_EXITSIZEMOVE:
        self->FinishRedockTracking();
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(hwnd, SW_MINIMIZE);
            return 0;
        }
        if ((wParam & 0xFFF0) == SC_RESTORE) {
            ShowWindow(hwnd, SW_RESTORE);
            return 0;
        }
        if ((wParam & 0xFFF0) == SC_MAXIMIZE) {
            ShowWindow(hwnd, SW_MAXIMIZE);
            return 0;
        }
        break;
    case WM_NCLBUTTONDOWN:
        if (wParam == HTCLOSE) {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        if (wParam == HTMAXBUTTON) {
            SendMessageW(hwnd, WM_SYSCOMMAND, IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
            return 0;
        }
        if (wParam == HTMINBUTTON) {
            SendMessageW(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            return 0;
        }
        break;
    case WM_NCLBUTTONDBLCLK:
        if (wParam == HTMAXBUTTON || wParam == HTCAPTION) {
            SendMessageW(hwnd, WM_SYSCOMMAND, IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
            return 0;
        }
        break;
    case WM_CUI_FLOAT_REDOCK: {
        DockManager* mgr = self->m_manager;
        if (!mgr) {
            return 0;
        }
        const Point pt = self->CursorScreenDip();
        mgr->CompleteFloatRedock(pt);
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
        self->HandleMouse(msg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSELEAVE:
        self->HandleMouse(WM_MOUSELEAVE, 0, 0);
        return 0;
    case WM_MOUSEWHEEL: {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);
        const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
        self->HandleWheel(delta, pt.x, pt.y);
        return 0;
    }
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && self->m_hovered) {
            if (HCURSOR cursor = self->m_hovered->GetCursor()) {
                SetCursor(cursor);
                return TRUE;
            }
        }
        break;
    case WM_CLOSE: {
        auto cb = self->m_onClose;
        self->m_onClose = nullptr;
        if (cb) {
            cb(self);
        }
        return 0;
    }
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
