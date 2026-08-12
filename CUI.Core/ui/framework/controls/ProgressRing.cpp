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
constexpr float kHalfPi = 1.57079632679f;
constexpr int kArcSegments = 64;
// WinUI ProgressRing indeterminate Lottie cycle length.
constexpr float kIndeterminateCycleSec = 2.0f;
// Visible arc length as a fraction of the circle (Fluent look).
constexpr float kMinSweepFrac = 0.08f;
constexpr float kMaxSweepFrac = 0.75f;

float NormalizeAngle(float radians) {
    while (radians < 0.0f) {
        radians += kTwoPi;
    }
    while (radians >= kTwoPi) {
        radians -= kTwoPi;
    }
    return radians;
}

float EaseInOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    const float u = -2.0f * t + 2.0f;
    return 1.0f - (u * u * u) * 0.5f;
}

// Rotation keyframes matching WinUI ProgressRingIndeterminate feel:
// 0% → 0°, 50% → 450°, 100% → 1080° (3 turns), with cubic ease between.
float WinUIRotationTurns(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= 0.5f) {
        return 1.25f * EaseInOutCubic(t / 0.5f);
    }
    return 1.25f + 1.75f * EaseInOutCubic((t - 0.5f) / 0.5f);
}

float WinUISweepFraction(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float u = (t < 0.5f)
        ? EaseInOutCubic(t * 2.0f)
        : EaseInOutCubic(2.0f - t * 2.0f);
    return kMinSweepFrac + (kMaxSweepFrac - kMinSweepFrac) * u;
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
    if (ind) {
        m_cycleTime = 0.0f;
        RequestAnimationTicks();
    }
    MarkRenderRectDirty(m_bounds);
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

    const int segments = (std::max)(6, static_cast<int>(std::ceil(kArcSegments * std::abs(sweepRad) / kTwoPi)));
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

void ProgressRing::SampleIndeterminate(float& outStartRad, float& outSweepRad) const {
    const float t = std::fmod(m_cycleTime, kIndeterminateCycleSec) / kIndeterminateCycleSec;
    const float sweepFrac = WinUISweepFraction(t);
    const float rotationRad = WinUIRotationTurns(t) * kTwoPi;
    // 12 o'clock origin, clockwise visual via increasing angle in screen space
    // (Y grows downward, so +angle is clockwise from +X — subtract π/2 for top).
    outSweepRad = sweepFrac * kTwoPi;
    outStartRad = NormalizeAngle(rotationRad - kHalfPi - outSweepRad);
}

void ProgressRing::OnRender(GraphicsContext& ctx) {
    if (m_bounds.IsEmpty()) {
        return;
    }

    const D2D1_COLOR_F accent = ResolveThemeColor(GetFillColorToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F track = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::CardBorder);
    track.a *= 0.55f;

    const float size = (std::min)(m_bounds.width, m_bounds.height);
    const float stroke = (std::max)(2.0f, size * 0.08f);
    const float radius = (std::max)(2.0f, size * 0.5f - stroke);
    const Point center(
        m_bounds.x + m_bounds.width * 0.5f,
        m_bounds.y + m_bounds.height * 0.5f);

    DrawRingArc(ctx, center, radius, stroke, track, -kHalfPi, kTwoPi);

    if (IsIndeterminate()) {
        float start = 0.0f;
        float sweep = 0.0f;
        SampleIndeterminate(start, sweep);
        DrawRingArc(ctx, center, radius, stroke, accent, start, sweep);
        return;
    }

    const float range = (std::max)(0.0001f, GetMaximum() - GetMinimum());
    const float progress = std::clamp((m_displayValue - GetMinimum()) / range, 0.0f, 1.0f);
    const float sweep = kTwoPi * progress;
    if (sweep > 0.001f) {
        DrawRingArc(ctx, center, radius, stroke, accent, -kHalfPi, sweep);
    }
}

bool ProgressRing::OnAnimationTick() {
    const float dt = UIElement::GetAnimationDeltaSeconds();
    bool animating = false;

    if (IsIndeterminate()) {
        if (m_visibility == Visibility::Visible && !m_bounds.IsEmpty()) {
            m_cycleTime += dt;
            if (m_cycleTime >= kIndeterminateCycleSec) {
                m_cycleTime = std::fmod(m_cycleTime, kIndeterminateCycleSec);
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
