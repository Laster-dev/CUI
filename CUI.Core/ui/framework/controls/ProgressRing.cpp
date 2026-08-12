#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ProgressRing.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {
constexpr float kTwoPi = 6.28318530718f;
constexpr int kArcSegments = 48;

float NormalizeAngle(float radians) {
    while (radians < 0.0f) {
        radians += kTwoPi;
    }
    while (radians >= kTwoPi) {
        radians -= kTwoPi;
    }
    return radians;
}
} // namespace

ProgressRing::ProgressRing() {
    SetFillColorToken(ThemeTokenId::AccentColor);
    SetTrackColorToken(ThemeTokenId::CardBorder);
    SetWidth(32.0f);
    SetHeight(32.0f);
    SetHoverBackgroundToken(ThemeTokenId::Unset);
    SetHoverBackground(D2D1::ColorF(0, 0, 0, 0));
    SetBackground(D2D1::ColorF(0, 0, 0, 0));
    m_displayValue = GetValue();
}

std::vector<PropertyMeta> ProgressRing::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "value", "进度数值 (Value)", "进度配置", "number" });
    metas.push_back({ "minimum", "最小值 (Minimum)", "进度配置", "number" });
    metas.push_back({ "maximum", "最大值 (Maximum)", "进度配置", "number" });
    metas.push_back({ "isIndeterminate", "不确定模式 (IsIndeterminate)", "进度配置", "bool" });
    return metas;
}

void ProgressRing::SetValue(float val) {
    m_value = val;
    NotifyFieldChanged(PropertyId::ControlValue, Value(val));
    if (!UIElement::AreAnimationsEnabled()) {
        m_displayValue = val;
    }
    MarkRenderRectDirty(m_bounds);
    RequestAnimationTicks();
}

void ProgressRing::SetMinimum(float minVal) {
    m_minimum = minVal;
    NotifyFieldChanged(PropertyId::Minimum, Value(minVal));
    MarkRenderRectDirty(m_bounds);
}

void ProgressRing::SetMaximum(float maxVal) {
    m_maximum = maxVal;
    NotifyFieldChanged(PropertyId::Maximum, Value(maxVal));
    MarkRenderRectDirty(m_bounds);
}

void ProgressRing::SetIsIndeterminate(bool ind) {
    if (m_isIndeterminate == ind) {
        return;
    }
    m_isIndeterminate = ind;
    NotifyFieldChanged(PropertyId::IsIndeterminate, Value(ind));
    MarkRenderRectDirty(m_bounds);
    if (ind) {
        RequestAnimationTicks();
    }
}

Size ProgressRing::Measure(Size availableSize) {
    (void)availableSize;
    float w = GetWidth() >= 0.0f ? GetWidth() : 32.0f;
    float h = GetHeight() >= 0.0f ? GetHeight() : 32.0f;
    m_desiredSize = Size(w, h);
    m_measureDirty = false;
    return m_desiredSize;
}

void ProgressRing::DrawRingArc(
    GraphicsContext& ctx,
    Point center,
    float radius,
    float strokeWidth,
    D2D1_COLOR_F color,
    float startRad,
    float sweepRad) const {
    if (radius <= 0.5f || std::abs(sweepRad) < 0.001f || color.a <= 0.001f) {
        return;
    }

    const int segments = (std::max)(4, static_cast<int>(std::ceil(kArcSegments * std::abs(sweepRad) / kTwoPi)));
    Point prev(
        center.x + std::cos(startRad) * radius,
        center.y + std::sin(startRad) * radius);

    for (int i = 1; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = startRad + sweepRad * t;
        Point next(
            center.x + std::cos(angle) * radius,
            center.y + std::sin(angle) * radius);
        ctx.DrawSmoothLine(prev, next, color, strokeWidth);
        prev = next;
    }
}

void ProgressRing::OnRender(GraphicsContext& ctx) {
    if (m_bounds.IsEmpty()) {
        return;
    }

    const auto& tokens = ThemeManager::Instance().GetTokens();
    const D2D1_COLOR_F accent = ResolveThemeColor(GetFillColorToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F track = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::CardBorder);
    track.a *= 0.55f;

    const float size = (std::min)(m_bounds.width, m_bounds.height);
    const float stroke = (std::max)(2.0f, size * 0.08f);
    const float radius = (std::max)(2.0f, size * 0.5f - stroke);
    const Point center(
        m_bounds.x + m_bounds.width * 0.5f,
        m_bounds.y + m_bounds.height * 0.5f);

    DrawRingArc(ctx, center, radius, stroke, track, -kTwoPi * 0.25f, kTwoPi);

    if (IsIndeterminate()) {
        const float sweep = kTwoPi * 0.72f;
        const float start = NormalizeAngle(m_spinAngle - kTwoPi * 0.25f);
        DrawRingArc(ctx, center, radius, stroke, accent, start, sweep);
        return;
    }

    const float range = (std::max)(0.0001f, GetMaximum() - GetMinimum());
    const float progress = std::clamp((m_displayValue - GetMinimum()) / range, 0.0f, 1.0f);
    const float sweep = kTwoPi * progress;
    if (sweep > 0.001f) {
        DrawRingArc(ctx, center, radius, stroke, accent, -kTwoPi * 0.25f, sweep);
    }
}

bool ProgressRing::OnAnimationTick() {
    const float dt = UIElement::GetAnimationDeltaSeconds();
    bool animating = false;

    if (IsIndeterminate()) {
        if (m_visibility == Visibility::Visible && !m_bounds.IsEmpty()) {
            m_spinAngle += kTwoPi * dt * 0.85f;
            if (m_spinAngle > kTwoPi) {
                m_spinAngle = std::fmod(m_spinAngle, kTwoPi);
            }
            MarkRenderRectDirty(m_bounds);
            animating = true;
        }
    } else if (UIElement::AreAnimationsEnabled()) {
        const float target = GetValue();
        const float delta = target - m_displayValue;
        if (std::abs(delta) > 0.01f) {
            m_displayValue += delta * (1.0f - std::exp(-10.0f * dt));
            MarkRenderRectDirty(m_bounds);
            animating = true;
        } else {
            m_displayValue = target;
        }
    } else {
        m_displayValue = GetValue();
    }

    if (animating) {
        RequestAnimationTicks();
    }
    return animating;
}

bool ProgressRing::HasSelfAnimation() const {
    if (m_visibility != Visibility::Visible || m_bounds.IsEmpty()) {
        return false;
    }
    return IsIndeterminate()
        || (UIElement::AreAnimationsEnabled() && std::abs(GetValue() - m_displayValue) > 0.01f);
}

void ProgressRing::OnNavigatedTo() {
    UIElement::OnNavigatedTo();
    if (IsIndeterminate() || std::abs(GetValue() - m_displayValue) > 0.01f) {
        RequestAnimationTicks();
    }
}

} // namespace CUI
