#include "Control.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {

Control::Control() {
    m_hoverBackgroundColor = D2D1::ColorF(0, 0, 0, 0);
    m_pressedBackgroundColor = D2D1::ColorF(0, 0, 0, 0);
    m_hasHoverBackgroundColor = true;
    m_hasPressedBackgroundColor = true;
}

Color Control::BlendColor(Color from, Color to, float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    return Color(
        from.r + (to.r - from.r) * t,
        from.g + (to.g - from.g) * t,
        from.b + (to.b - from.b) * t,
        from.a + (to.a - from.a) * t
    );
}

void Control::UpdateVisualStateTarget() {
    m_visualStateTarget = GetVisualStateTarget();
}

float Control::GetVisualStateTarget() const {
    if (!IsEnabled()) {
        return 0.0f;
    }
    if (m_isPressed) {
        return 1.0f;
    }
    if (m_isHovered || m_isFocused) {
        return 0.55f;
    }
    return 0.0f;
}

bool Control::VisualStateChromeDiffers() const {
    Color bg = m_hasBackgroundColor
        ? m_backgroundColor
        : ((m_backgroundToken != ThemeTokenId::Unset)
            ? ResolveThemeColor(m_backgroundToken, ThemeTokenId::CardBackground)
            : Color::Transparent);
    Color hoverBg = m_hasHoverBackgroundColor
        ? m_hoverBackgroundColor
        : ((m_hoverBackgroundToken != ThemeTokenId::Unset)
            ? ResolveThemeColor(m_hoverBackgroundToken, ThemeTokenId::HoverBackground)
            : bg);
    return std::abs(bg.r - hoverBg.r) > 0.002f
        || std::abs(bg.g - hoverBg.g) > 0.002f
        || std::abs(bg.b - hoverBg.b) > 0.002f
        || std::abs(bg.a - hoverBg.a) > 0.002f;
}

Color Control::GetAnimatedBackground(Color fallback) {
    Color bg = m_hasBackgroundColor
        ? m_backgroundColor
        : ((m_backgroundToken != ThemeTokenId::Unset)
            ? ResolveThemeColor(m_backgroundToken, ThemeTokenId::CardBackground)
            : fallback);

    Color hoverBg = m_hasHoverBackgroundColor
        ? m_hoverBackgroundColor
        : ((m_hoverBackgroundToken != ThemeTokenId::Unset)
            ? ResolveThemeColor(m_hoverBackgroundToken, ThemeTokenId::HoverBackground)
            : bg);

    Color pressedBg = m_hasPressedBackgroundColor
        ? m_pressedBackgroundColor
        : ((m_pressedBackgroundToken != ThemeTokenId::Unset)
            ? ResolveThemeColor(m_pressedBackgroundToken, ThemeTokenId::PressedBackground)
            : hoverBg);

    // Disabled uses the resting fill; UIElement::Render fades the whole control.
    if (!IsEnabled()) {
        return bg;
    }
    float visualState = m_visualStateAnim.Current();
    if (visualState <= 0.0f) return bg;
    if (visualState <= 0.55f) return BlendColor(bg, hoverBg, visualState / 0.55f);
    return BlendColor(hoverBg, pressedBg, (visualState - 0.55f) / 0.45f);
}

bool Control::OnAnimationTick() {
    bool childAnimating = UIElement::OnAnimationTick();
    UpdateVisualStateTarget();
    m_visualStateAnim.ApplyTarget(m_visualStateTarget);
    const float prev = m_visualStateAnim.Current();
    // Finite maxDuration (~CSS transition) + larger epsilon — browser-like.
    const bool selfAnimating = m_visualStateAnim.Tick(
        UIElement::GetAnimationDeltaSeconds(),
        AnimationSpec{ 0.40f, 0.04f });
    if (selfAnimating || std::abs(m_visualStateAnim.Current() - prev) > 0.004f) {
        MarkRenderRectDirty(m_bounds);
    }
    if (selfAnimating) {
        RequestAnimationTicks();
    }
    return childAnimating || selfAnimating;
}

bool Control::HasSelfAnimation() const {
    return m_visualStateAnim.IsAnimating(0.04f);
}

void Control::OnRender(GraphicsContext& ctx) {
    float radius = GetCornerRadius();
    Color bg = GetAnimatedBackground(Color::Transparent);

    if (bg.a > 0.0f) {
        if (radius > 0.0f) {
            ctx.FillRoundedRect(m_bounds, radius, bg);
        } else {
            ctx.FillRect(m_bounds, bg);
        }
    }

    Color borderBrush = m_hasBorderBrushColor
        ? m_borderBrushColor
        : ((m_borderToken != ThemeTokenId::Unset)
            ? ResolveThemeColor(m_borderToken, ThemeTokenId::CardBorder)
            : Color::Transparent);
    float borderThickness = GetBorderThickness();
    if (borderBrush.a > 0.0f && borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, borderBrush, borderThickness);
        } else {
            ctx.DrawRect(m_bounds, borderBrush, borderThickness);
        }
    }
}

void Control::OnMouseEnter() {
    if (!IsEnabled()) return;
    UIElement::OnMouseEnter();
    UpdateVisualStateTarget();
    // Instant chrome (no multi-frame color re-raster). Continuous hover fades were
    // the main "slide mouse → CPU spike" source across dozens of controls.
    m_visualStateAnim.Reset(m_visualStateTarget);
    if (VisualStateChromeDiffers()) {
        MarkRenderRectDirty(m_bounds);
    }
}

void Control::OnMouseLeave() {
    UIElement::OnMouseLeave();
    UpdateVisualStateTarget();
    m_visualStateAnim.Reset(m_visualStateTarget);
    if (VisualStateChromeDiffers()) {
        MarkRenderRectDirty(m_bounds);
    }
}

void Control::OnMouseDown(Point pt) {
    if (!IsEnabled()) return;
    UIElement::OnMouseDown(pt);
    UpdateVisualStateTarget();
    m_visualStateAnim.ApplyTarget(m_visualStateTarget);
    MarkRenderRectDirty(m_bounds);
    RequestAnimationTicks();
}

void Control::OnMouseUp(Point pt) {
    UIElement::OnMouseUp(pt);
    UpdateVisualStateTarget();
    m_visualStateAnim.ApplyTarget(m_visualStateTarget);
    MarkRenderRectDirty(m_bounds);
    RequestAnimationTicks();
}

void Control::OnMouseMove(Point pt) {
    UIElement::OnMouseMove(pt);
}
void Control::OnFocus() {
    UIElement::OnFocus();
    UpdateVisualStateTarget();
    // Instant chrome — same as hover enter/leave. Animating focus-in would replay
    // a bright→dark fade on unrelated actions (e.g. clicking elsewhere to blur).
    m_visualStateAnim.Reset(m_visualStateTarget);
    if (VisualStateChromeDiffers()) {
        MarkRenderRectDirty(m_bounds);
    }
}

void Control::OnBlur() {
    UIElement::OnBlur();
    UpdateVisualStateTarget();
    // Instant chrome removal: no animated fade on focus loss.
    m_visualStateAnim.Reset(m_visualStateTarget);
    if (VisualStateChromeDiffers()) {
        MarkRenderRectDirty(m_bounds);
    }
}

} // namespace CUI
