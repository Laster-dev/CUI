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
// WinUI ProgressRingIndeterminate Lottie duration (c_durationTicks = 20_000_000).
constexpr float kIndeterminateCycleSec = 2.0f;

float CubicBezierSample(float t, float a, float b, float c, float d) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float u = 1.0f - t;
    const float u2 = u * u;
    const float u3 = u2 * u;
    return u3 * a + 3.0f * u2 * t * b + 3.0f * u * t2 * c + t3 * d;
}

// Solve cubic-bezier(x1,y1,x2,y2) for progress t ∈ [0,1].
float CubicBezierEase(float t, float x1, float y1, float x2, float y2) {
    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= 0.0f || t >= 1.0f) {
        return t;
    }
    float s = t;
    for (int i = 0; i < 8; ++i) {
        const float x = CubicBezierSample(s, 0.0f, x1, x2, 1.0f);
        const float dx =
            3.0f * (1.0f - s) * (1.0f - s) * x1
            + 6.0f * (1.0f - s) * s * (x2 - x1)
            + 3.0f * s * s * (1.0f - x2);
        if (std::abs(dx) < 1.0e-6f) {
            break;
        }
        s -= (x - t) / dx;
        s = std::clamp(s, 0.0f, 1.0f);
    }
    return CubicBezierSample(s, 0.0f, y1, y2, 1.0f);
}

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
    ctx.DrawSmoothArc(center, radius, startRad, sweepRad, color, strokeWidth);
}

void ProgressRing::SampleIndeterminate(float& outStartRad, float& outSweepRad) const {
    // Structure from WinUI ProgressRingIndeterminate (2s, trim chase, 0→450→900),
    // but rotation uses a real ease-in-out — WinUI's published bezier
    // (0.167,0.167)-(0.833,0.833) lies on y=x and reads as constant RPM.
    const float t = std::fmod(m_cycleTime, kIndeterminateCycleSec) / kIndeterminateCycleSec;

    // Emphasized ease-in-out: slow → fast → slow within each half-cycle.
    constexpr float kRotX1 = 0.45f, kRotY1 = 0.05f, kRotX2 = 0.55f, kRotY2 = 0.95f;
    // Trim keeps a slightly softer curve so arc length still "breathes".
    constexpr float kTrimX1 = 0.33f, kTrimY1 = 0.00f, kTrimX2 = 0.67f, kTrimY2 = 1.00f;

    float rotationDeg = 0.0f;
    float trimStart = 0.0f;
    float trimEnd = 0.0001f;

    if (t < 0.5f) {
        const float local = t / 0.5f;
        const float rotU = CubicBezierEase(local, kRotX1, kRotY1, kRotX2, kRotY2);
        const float trimU = CubicBezierEase(local, kTrimX1, kTrimY1, kTrimX2, kTrimY2);
        rotationDeg = 450.0f * rotU;
        trimStart = 0.0f;
        trimEnd = 0.0001f + (0.5f - 0.0001f) * trimU;
    } else {
        const float local = (t - 0.5f) / 0.5f;
        const float rotU = CubicBezierEase(local, kRotX1, kRotY1, kRotX2, kRotY2);
        const float trimU = CubicBezierEase(local, kTrimX1, kTrimY1, kTrimX2, kTrimY2);
        rotationDeg = 450.0f + 450.0f * rotU;
        trimStart = 0.5f * trimU;
        trimEnd = 0.5f;
    }

    const float rotationRad = rotationDeg * (kTwoPi / 360.0f);
    outSweepRad = (trimEnd - trimStart) * kTwoPi;
    outStartRad = NormalizeAngle(rotationRad + trimStart * kTwoPi - kHalfPi);
}

void ProgressRing::OnRender(GraphicsContext& ctx) {
    if (m_bounds.IsEmpty()) {
        return;
    }

    const D2D1_COLOR_F accent = ResolveThemeColor(GetFillColorToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F track = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::CardBorder);
    track.a *= 0.55f;

    const float size = (std::min)(m_bounds.width, m_bounds.height);
    // WinUI Lottie: radius 7, stroke 1.5, scale 5 → stroke ≈ 0.094 of outer size.
    const float stroke = (std::max)(2.0f, size * 0.094f);
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
