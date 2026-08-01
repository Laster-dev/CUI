#include "Control.h"
#include <algorithm>

namespace CUI {

Control::Control() {
    SetProperty("hoverBackground", Value(D2D1::ColorF(0, 0, 0, 0)));
    SetProperty("pressedBackground", Value(D2D1::ColorF(0, 0, 0, 0)));
}

D2D1_COLOR_F Control::BlendColor(D2D1_COLOR_F from, D2D1_COLOR_F to, float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    return D2D1::ColorF(
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

D2D1_COLOR_F Control::GetAnimatedBackground(D2D1_COLOR_F fallback) {
    D2D1_COLOR_F bg = GetProperty("background").AsColor(fallback);
    D2D1_COLOR_F hoverBg = GetProperty("hoverBackground").AsColor(bg);
    D2D1_COLOR_F pressedBg = GetProperty("pressedBackground").AsColor(hoverBg);
    D2D1_COLOR_F disabledBg = GetProperty("disabledBackground").AsColor(D2D1::ColorF(0x28 / 255.0f, 0x28 / 255.0f, 0x28 / 255.0f, 0.6f));

    if (!IsEnabled()) return disabledBg;
    float visualState = m_visualStateAnim.Current();
    if (visualState <= 0.0f) return bg;
    if (visualState <= 0.55f) return BlendColor(bg, hoverBg, visualState / 0.55f);
    return BlendColor(hoverBg, pressedBg, (visualState - 0.55f) / 0.45f);
}

bool Control::OnAnimationTick() {
    bool childAnimating = UIElement::OnAnimationTick();
    UpdateVisualStateTarget();
    m_visualStateAnim.SetTarget(m_visualStateTarget);
    const bool selfAnimating = m_visualStateAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.28f, 0.01f });
    return childAnimating || selfAnimating;
}

bool Control::HasSelfAnimation() const {
    return m_visualStateAnim.IsAnimating(0.01f);
}

void Control::OnRender(GraphicsContext& ctx) {
    float radius = GetProperty("cornerRadius").AsFloat(0.0f);
    D2D1_COLOR_F bg = GetAnimatedBackground(D2D1::ColorF(0, 0, 0, 0));

    if (bg.a > 0.0f) {
        if (radius > 0.0f) {
            ctx.FillRoundedRect(m_bounds, radius, bg);
        } else {
            ctx.FillRect(m_bounds, bg);
        }
    }

    D2D1_COLOR_F borderBrush = IsEnabled()
        ? GetProperty("borderBrush").AsColor(D2D1::ColorF(0, 0, 0, 0))
        : D2D1::ColorF(0x3A / 255.0f, 0x3A / 255.0f, 0x3A / 255.0f, 0.5f);
    float borderThickness = GetProperty("borderThickness").AsFloat(0.0f);
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
}

void Control::OnMouseLeave() {
    UIElement::OnMouseLeave();
    UpdateVisualStateTarget();
}

void Control::OnMouseDown(Point pt) {
    if (!IsEnabled()) return;
    UIElement::OnMouseDown(pt);
    UpdateVisualStateTarget();
}

void Control::OnMouseUp(Point pt) {
    if (!IsEnabled()) return;
    UIElement::OnMouseUp(pt);
    UpdateVisualStateTarget();
}

void Control::OnMouseMove(Point pt) {
    if (!IsEnabled()) return;
    UIElement::OnMouseMove(pt);
}

} // namespace CUI
