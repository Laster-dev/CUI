#include "WindowTitleBar.h"

#include "../style/ThemeManager.h"
#include "../window/Dpi.h"
#include "../window/Window.h"

#include <algorithm>
#include <cctype>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace CUI {

namespace {

constexpr float kCaptionButtonWidth = 46.0f;
constexpr float kIconSize = 18.0f;
// Match MenuBar hover: ease-out over ~160ms when animations are enabled.
constexpr AnimationSpec kCaptionHoverSpec{ 0.22f, 0.01f, 0.16f };

D2D1_COLOR_F LerpColor(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return D2D1::ColorF(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t);
}

std::string DefaultIconTextFromTitle(const std::string& title) {
    for (unsigned char ch : title) {
        if (std::isalnum(ch)) {
            return std::string(1, static_cast<char>(std::toupper(ch)));
        }
    }
    return "C";
}

} // namespace

WindowTitleBar::WindowTitleBar() : RightContent(this), MenuBar(this) {
    SetHeight(36.0f);
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetHoverBackgroundToken(ThemeTokenId::PaneBackground);
    SetPressedBackgroundToken(ThemeTokenId::PaneBackground);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetTitle("CUI Application");
    m_menuBar = std::make_shared<CUI::MenuBar>();
    AddChild(m_menuBar);
}

WindowTitleBar::~WindowTitleBar() {
    if (m_ownsNativeIcon && m_nativeIcon) {
        DestroyIcon(m_nativeIcon);
        m_nativeIcon = nullptr;
    }
}

void WindowTitleBar::SetTitle(const std::string& title) {
    m_title = title;
    if (m_iconText.empty()) {
        m_iconText = DefaultIconTextFromTitle(title);
    }
    MarkRenderContentDirty();
}

void WindowTitleBar::SetIconText(const std::string& iconText) {
    m_iconText = iconText.empty() ? DefaultIconTextFromTitle(m_title) : iconText;
    MarkRenderContentDirty();
}

void WindowTitleBar::SetNativeIcon(HICON icon, bool takeOwnership) {
    if (m_ownsNativeIcon && m_nativeIcon && m_nativeIcon != icon) {
        DestroyIcon(m_nativeIcon);
    }
    m_nativeIcon = icon;
    m_ownsNativeIcon = takeOwnership && icon != nullptr;
    m_nativeIconBitmap.Reset();
    MarkRenderContentDirty();
}

bool WindowTitleBar::EnsureNativeIconBitmap(GraphicsContext& ctx) {
    if (!m_nativeIcon) return false;
    if (m_nativeIconBitmap) return true;

    auto* d2d = ctx.GetD2DContext();
    if (!d2d) return false;

    ICONINFO ii = {};
    if (!GetIconInfo(m_nativeIcon, &ii)) return false;

    BITMAP bmp = {};
    if (ii.hbmColor) {
        GetObject(ii.hbmColor, sizeof(bmp), &bmp);
    } else if (ii.hbmMask) {
        GetObject(ii.hbmMask, sizeof(bmp), &bmp);
        bmp.bmHeight /= 2;
    }

    const int w = (std::max)(1, static_cast<int>(bmp.bmWidth));
    const int h = (std::max)(1, static_cast<int>(bmp.bmHeight > 0 ? bmp.bmHeight : bmp.bmWidth));

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<UINT32> pixels(static_cast<size_t>(w) * static_cast<size_t>(h), 0);
    HDC screenDc = GetDC(nullptr);
    HDC memDc = CreateCompatibleDC(screenDc);
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(memDc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    bool ok = false;
    if (dib && memDc && bits) {
        HGDIOBJ old = SelectObject(memDc, dib);
        DrawIconEx(memDc, 0, 0, m_nativeIcon, w, h, 0, nullptr, DI_NORMAL);
        memcpy(pixels.data(), bits, pixels.size() * sizeof(UINT32));
        SelectObject(memDc, old);
        ok = true;
    }
    if (dib) DeleteObject(dib);
    if (memDc) DeleteDC(memDc);
    if (screenDc) ReleaseDC(nullptr, screenDc);
    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);
    if (!ok) return false;

    for (UINT32& px : pixels) {
        const UINT32 a = (px >> 24) & 0xFFu;
        if (a == 0 || a == 255) continue;
        const UINT32 r = (px >> 16) & 0xFFu;
        const UINT32 g = (px >> 8) & 0xFFu;
        const UINT32 b = px & 0xFFu;
        px = (a << 24)
            | (((r * a) / 255u) << 16)
            | (((g * a) / 255u) << 8)
            | ((b * a) / 255u);
    }

    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
    const HRESULT hr = d2d->CreateBitmap(
        D2D1::SizeU(static_cast<UINT32>(w), static_cast<UINT32>(h)),
        pixels.data(),
        static_cast<UINT32>(w * 4),
        &props,
        &bitmap
    );
    if (FAILED(hr) || !bitmap) return false;
    m_nativeIconBitmap = bitmap;
    return true;
}

Rect WindowTitleBar::GetMinimizeButtonRect() const {
    const float right = m_bounds.x + m_bounds.width;
    return Rect(right - kCaptionButtonWidth * 3.0f, m_bounds.y, kCaptionButtonWidth, m_bounds.height);
}

Rect WindowTitleBar::GetMaximizeButtonRect() const {
    const float right = m_bounds.x + m_bounds.width;
    return Rect(right - kCaptionButtonWidth * 2.0f, m_bounds.y, kCaptionButtonWidth, m_bounds.height);
}

Rect WindowTitleBar::GetCloseButtonRect() const {
    const float right = m_bounds.x + m_bounds.width;
    return Rect(right - kCaptionButtonWidth, m_bounds.y, kCaptionButtonWidth, m_bounds.height);
}

void WindowTitleBar::SetRightContent(const std::shared_ptr<UIElement>& content) {
    if (m_rightContent == content) {
        return;
    }
    if (m_rightContent) {
        RemoveChild(m_rightContent);
    }
    m_rightContent = content;
    if (m_rightContent) {
        AddChild(m_rightContent);
    }
}

bool WindowTitleBar::IsMenuBarHit(float x, float y) const {
    return m_menuBar && m_menuBar->HitTest(x, y) != nullptr;
}

bool WindowTitleBar::IsRightContentHit(float x, float y) const {
    return m_rightContent && m_rightContent->HitTest(x, y) != nullptr;
}
bool WindowTitleBar::IsCaptionButtonHit(float x, float y) const {
    return GetMinimizeButtonRect().Contains(x, y)
        || GetMaximizeButtonRect().Contains(x, y)
        || GetCloseButtonRect().Contains(x, y);
}

int WindowTitleBar::HitTestHoverRegion(float x, float y) const {
    if (GetCloseButtonRect().Contains(x, y)) {
        return 3;
    }
    if (GetMaximizeButtonRect().Contains(x, y)) {
        return 2;
    }
    if (GetMinimizeButtonRect().Contains(x, y)) {
        return 1;
    }
    if (IsMenuBarHit(x, y)) {
        return 0;
    }
    return -1;
}

void WindowTitleBar::SyncCaptionHoverTargets() {
    const float minT = (m_hoverRegion == 1) ? 1.0f : 0.0f;
    const float maxT = (m_hoverRegion == 2) ? 1.0f : 0.0f;
    const float closeT = (m_hoverRegion == 3) ? 1.0f : 0.0f;
    m_minHoverAnim.SetTarget(minT);
    m_maxHoverAnim.SetTarget(maxT);
    m_closeHoverAnim.SetTarget(closeT);
    if (!AreAnimationsEnabled()) {
        m_minHoverAnim.Reset(minT);
        m_maxHoverAnim.Reset(maxT);
        m_closeHoverAnim.Reset(closeT);
    } else {
        RequestAnimationTicks();
    }
}

void WindowTitleBar::ApplyHoverRegion(int region, bool forceDirty) {
    const int previous = m_hoverRegion;
    if (!forceDirty && previous == region) {
        return;
    }
    m_hoverRegion = region;
    SyncCaptionHoverTargets();
    if (forceDirty || previous != m_hoverRegion || HasSelfAnimation()) {
        m_menuChromeDirty = true;
        if (!m_bounds.IsEmpty()) {
            MarkRenderRectDirty(m_bounds.Inflate(2.0f));
        }
    }
}

void WindowTitleBar::NotifyNonClientMouseMove(float x, float y) {
    ApplyHoverRegion(HitTestHoverRegion(x, y), false);
}

void WindowTitleBar::NotifyNonClientMouseLeave() {
    ApplyHoverRegion(-1, true);
}

void WindowTitleBar::OnThemeChanged() {
    Control::OnThemeChanged();
    // Device resources may have been dropped on theme switch.
    m_nativeIconBitmap.Reset();
}

void WindowTitleBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    const bool lightTheme = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;
    const ::HWND hwnd = ctx.GetHwnd();
    const bool isMaximized = hwnd && IsZoomed(hwnd) != FALSE;

    const Rect iconRect(
        m_bounds.x + 10.0f,
        m_bounds.y + (m_bounds.height - kIconSize) * 0.5f,
        kIconSize,
        kIconSize
    );

    bool drewNative = false;
    if (m_nativeIcon && EnsureNativeIconBitmap(ctx) && m_nativeIconBitmap && ctx.GetD2DContext()) {
        const D2D1_RECT_F dest = iconRect.ToD2D();
        ctx.GetD2DContext()->DrawBitmap(m_nativeIconBitmap.Get(), dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        drewNative = true;
    }
    if (!drewNative) {
        const std::string& icon = GetIcon().empty() ? (m_iconText.empty() ? DefaultIconTextFromTitle(m_title) : m_iconText) : GetIcon();
        if (GraphicsContext::LooksLikeSvg(icon)) {
            ctx.DrawIcon(icon, iconRect, tokens.accentForeground);
        } else {
            ctx.FillRoundedRect(iconRect, 4.0f, tokens.accentColor);
            ctx.DrawText(
                icon,
                iconRect,
                tokens.accentForeground,
                "微软雅黑",
                11.0f,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                DWRITE_FONT_WEIGHT_BOLD
            );
        }
    }

    const Rect menuBarRect = LayoutMenuBar(ctx);

    const D2D1_COLOR_F titleColor = tokens.textPrimary;
    const float captionHoverPeak = lightTheme ? 0.08f : 0.14f;

    const float titleLeft = menuBarRect.x + menuBarRect.width + 12.0f;
    const float titleRight = GetMinimizeButtonRect().x - 8.0f;
    if (titleRight > titleLeft) {
        ctx.DrawText(
            m_title,
            Rect(titleLeft, m_bounds.y, titleRight - titleLeft, m_bounds.height),
            titleColor,
            "微软雅黑",
            16.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_NORMAL,
            true
        );
    }

    const Rect minRect = GetMinimizeButtonRect();
    const Rect maxRect = GetMaximizeButtonRect();
    const Rect closeRect = GetCloseButtonRect();
    const float minHoverT = m_minHoverAnim.Current();
    const float maxHoverT = m_maxHoverAnim.Current();
    const float closeHoverT = m_closeHoverAnim.Current();

    if (minHoverT > 0.001f) {
        ctx.FillRect(minRect, D2D1::ColorF(
            tokens.textPrimary.r, tokens.textPrimary.g, tokens.textPrimary.b,
            captionHoverPeak * minHoverT));
    }
    if (maxHoverT > 0.001f) {
        ctx.FillRect(maxRect, D2D1::ColorF(
            tokens.textPrimary.r, tokens.textPrimary.g, tokens.textPrimary.b,
            captionHoverPeak * maxHoverT));
    }
    if (closeHoverT > 0.001f) {
        D2D1_COLOR_F closeBg = tokens.dangerColor;
        closeBg.a *= closeHoverT;
        ctx.FillRect(closeRect, closeBg);
    }

    const float iconCenterY = m_bounds.y + m_bounds.height * 0.5f;
    ctx.DrawLine(Point(minRect.x + 18.0f, iconCenterY), Point(minRect.x + 28.0f, iconCenterY), titleColor, 1.0f);

    if (isMaximized) {
        ctx.DrawRect(Rect(maxRect.x + 20.0f, iconCenterY - 4.0f, 8.0f, 8.0f), titleColor, 1.0f);
        ctx.DrawRect(Rect(maxRect.x + 18.0f, iconCenterY - 6.0f, 8.0f, 8.0f), titleColor, 1.0f);
    } else {
        ctx.DrawRect(Rect(maxRect.x + 18.0f, iconCenterY - 5.0f, 10.0f, 10.0f), titleColor, 1.0f);
    }

    const D2D1_COLOR_F closeIconColor = LerpColor(titleColor, tokens.accentForeground, closeHoverT);
    const float closeCenterX = closeRect.x + closeRect.width * 0.5f;
    ctx.DrawLine(Point(closeCenterX - 5.0f, iconCenterY - 5.0f), Point(closeCenterX + 5.0f, iconCenterY + 5.0f), closeIconColor, 1.2f);
    ctx.DrawLine(Point(closeCenterX + 5.0f, iconCenterY - 5.0f), Point(closeCenterX - 5.0f, iconCenterY + 5.0f), closeIconColor, 1.2f);
}

void WindowTitleBar::OnRenderOverlay(GraphicsContext& ctx) {
    // Hairline after children: MenuBar used to FillRect the full title height and
    // stamp over this line (UIElement paints parent OnRender, then children).
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    ctx.DrawLine(
        Point(m_bounds.x, m_bounds.y + m_bounds.height - 1.0f),
        Point(m_bounds.x + m_bounds.width, m_bounds.y + m_bounds.height - 1.0f),
        tokens.cardBorder);
}

Rect WindowTitleBar::LayoutMenuBar(GraphicsContext& ctx) {
    const float menuWidth = m_menuBar->GetTotalWidth(ctx);
    const Rect menuBarRect(
        m_bounds.x + 36.0f,
        m_bounds.y,
        menuWidth,
        (std::max)(0.0f, m_bounds.height - 1.0f));
    m_menuBar->Arrange(menuBarRect);
    return menuBarRect;
}

Rect WindowTitleBar::LayoutRightContent() {
    if (!m_rightContent) {
        return Rect();
    }

    const Rect minimize = GetMinimizeButtonRect();
    const float left = m_bounds.x + 36.0f;
    const float availableWidth = (std::max)(0.0f, minimize.x - left - 8.0f);
    const Size desired = m_rightContent->Measure(Size(availableWidth, m_bounds.height));
    const float width = (std::min)(desired.width, availableWidth);
    const float height = (std::min)(desired.height, (std::max)(0.0f, m_bounds.height - 6.0f));
    const Rect contentRect(
        minimize.x - 8.0f - width,
        m_bounds.y + (m_bounds.height - height) * 0.5f,
        width,
        height);
    m_rightContent->Arrange(contentRect);
    return contentRect;
}

void WindowTitleBar::Arrange(Rect finalRect) {
    UIElement::Arrange(finalRect);
    GraphicsContext ctx;
    LayoutRightContent();
    LayoutMenuBar(ctx);
}
void WindowTitleBar::ResetMenuInteraction() {
    m_menuBar->ResetInteractionState();
}

void WindowTitleBar::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    ApplyHoverRegion(HitTestHoverRegion(pt.x, pt.y), false);
}

void WindowTitleBar::OnMouseLeave() {
    Control::OnMouseLeave();
    // WM_MOUSELEAVE is client-only. Entering HTMIN/MAX/CLOSE still synthesizes it;
    // keep caption hover — WM_NCMOUSELEAVE clears when the cursor really leaves.
    float lx = 0.0f;
    float ly = 0.0f;
    ::HWND hwnd = Window::Current() ? Window::Current()->GetHWND() : nullptr;
    if (hwnd && TryGetCursorClientLogical(hwnd, lx, ly) && IsCaptionButtonHit(lx, ly)) {
        return;
    }
    ApplyHoverRegion(-1, true);
}

bool WindowTitleBar::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    bool any = false;
    if (m_minHoverAnim.Tick(dt, kCaptionHoverSpec)) {
        any = true;
    }
    if (m_maxHoverAnim.Tick(dt, kCaptionHoverSpec)) {
        any = true;
    }
    if (m_closeHoverAnim.Tick(dt, kCaptionHoverSpec)) {
        any = true;
    }
    if (any) {
        m_menuChromeDirty = true;
        if (!m_bounds.IsEmpty()) {
            MarkRenderRectDirty(m_bounds.Inflate(4.0f));
        }
        RequestAnimationTicks();
    }
    return base || any;
}

bool WindowTitleBar::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || m_minHoverAnim.IsAnimating()
        || m_maxHoverAnim.IsAnimating()
        || m_closeHoverAnim.IsAnimating();
}

bool WindowTitleBar::IsInteractiveHit(float x, float y) const {
    return IsMenuBarHit(x, y) || IsRightContentHit(x, y);
}

bool WindowTitleBar::IsCaptionDragHit(float x, float y, UIElement* treeHit) const {
    if (!m_bounds.Contains(x, y)) {
        return false;
    }
    if (IsInteractiveHit(x, y) || IsCaptionButtonHit(x, y)) {
        return false;
    }
    return !treeHit || treeHit == this;
}

LRESULT WindowTitleBar::HitTestNonClient(float x, float y) const {
    if (!m_bounds.Contains(x, y)) {
        return HTNOWHERE;
    }
    if (GetCloseButtonRect().Contains(x, y)) {
        return HTCLOSE;
    }
    if (GetMaximizeButtonRect().Contains(x, y)) {
        return HTMAXBUTTON;
    }
    if (GetMinimizeButtonRect().Contains(x, y)) {
        return HTMINBUTTON;
    }
    return HTNOWHERE;
}

bool WindowTitleBar::ConsumeChromeDirty() {
    const bool dirty = m_menuChromeDirty;
    m_menuChromeDirty = false;
    return dirty;
}

} // namespace CUI


namespace CUI {
std::shared_ptr<ContextMenu> WindowTitleBar::WindowTitleBarMenuBarProperty::AddMenu(const std::string& text) {
    return owner->GetMenuBar().AddMenu(text);
}
}
