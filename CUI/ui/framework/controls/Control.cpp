#include "Control.h"
#include "../style/ThemeManager.h"
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
    D2D1_COLOR_F bg = HasProperty("theme.backgroundToken")
        ? ResolveThemeColor("theme.backgroundToken", "cardBackground")
        : GetProperty("background").AsColor(fallback);
    // If hover/pressed tokens are absent, reuse live bg — never a stale RGB snapshot
    // from a previous theme (that caused TitleBar to go black on focus after Light switch).
    D2D1_COLOR_F hoverBg = HasProperty("theme.hoverBackgroundToken")
        ? ResolveThemeColor("theme.hoverBackgroundToken", "hoverBackground")
        : bg;
    D2D1_COLOR_F pressedBg = HasProperty("theme.pressedBackgroundToken")
        ? ResolveThemeColor("theme.pressedBackgroundToken", "pressedBackground")
        : hoverBg;
    D2D1_COLOR_F disabledBg = HasProperty("theme.disabledBackgroundToken")
        ? ResolveThemeColor("theme.disabledBackgroundToken", "hoverBackground")
        : GetProperty("disabledBackground").AsColor(ThemeManager::Instance().GetColor("hoverBackground"));
    disabledBg.a = (std::min)(disabledBg.a, 0.6f);

    if (HasProperty("chromeBackdropAlpha")) {
        const float a = std::clamp(GetProperty("chromeBackdropAlpha").AsFloat(1.0f), 0.0f, 1.0f);
        bg.a *= a;
        hoverBg.a *= a;
        pressedBg.a *= a;
    }

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
        ? (HasProperty("theme.borderToken")
            ? ResolveThemeColor("theme.borderToken", "cardBorder")
            : GetProperty("borderBrush").AsColor(D2D1::ColorF(0, 0, 0, 0)))
        : [&]() {
            D2D1_COLOR_F c = ThemeManager::Instance().GetColor("cardBorder");
            c.a = 0.5f;
            return c;
        }();
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
