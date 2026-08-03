#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ProgressBar.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace CUI {

static float WinUI3EaseInOut(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

ProgressBar::ProgressBar() {
    SetProperty("minimum", Value(0.0f));
    SetProperty("maximum", Value(100.0f));
    SetProperty("value", Value(0.0f));
    SetProperty("isIndeterminate", Value(false));
    SetProperty("fillColor", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("trackColor", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
    SetProperty("width", Value(200.0f));
    SetProperty("height", Value(3.0f));
    SetProperty("cornerRadius", Value(1.5f));
    m_displayValue = GetValue();
}

std::vector<PropertyMeta> ProgressBar::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "value", "进度数值 (Value)", "进度配置", "number" });
    metas.push_back({ "minimum", "最小值 (Minimum)", "进度配置", "number" });
    metas.push_back({ "maximum", "最大值 (Maximum)", "进度配置", "number" });
    metas.push_back({ "isIndeterminate", "不确定模式 (IsIndeterminate)", "进度配置", "bool" });
    metas.push_back({ "fillColor", "填充颜色 (FillColor)", "色彩外观", "color" });
    metas.push_back({ "trackColor", "轨道颜色 (TrackColor)", "色彩外观", "color" });
    return metas;
}

Size ProgressBar::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(200.0f);
    float expH = GetProperty("height").AsFloat(3.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

bool ProgressBar::OnAnimationTick() {
    bool baseAnim = Control::OnAnimationTick();

    const auto now = std::chrono::steady_clock::now();
    float deltaSeconds = 1.0f / 60.0f;
    if (m_lastTickTime.time_since_epoch().count() != 0) {
        deltaSeconds = std::chrono::duration<float>(now - m_lastTickTime).count();
        deltaSeconds = std::clamp(deltaSeconds, 1.0f / 240.0f, 0.05f);
    }
    m_lastTickTime = now;

    if (IsIndeterminate()) {
        const float speed = 1.0f;
        m_animOffset += speed * deltaSeconds;
        if (m_animOffset > 10000.0f) {
            m_animOffset = std::fmod(m_animOffset, 2.0f);
        }
        return true;
    }

    if (!UIElement::AreAnimationsEnabled()) {
        m_displayValue = GetValue();
        return baseAnim;
    }

    float target = GetValue();
    float delta = target - m_displayValue;
    if (std::abs(delta) <= 0.01f) {
        m_displayValue = target;
        return baseAnim;
    }
    float smoothing = 1.0f - std::exp(-12.0f * deltaSeconds);
    m_displayValue += delta * smoothing;
    return true;
}

bool ProgressBar::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || IsIndeterminate()
        || (UIElement::AreAnimationsEnabled() && std::abs(GetValue() - m_displayValue) > 0.01f);
}

void ProgressBar::OnRender(GraphicsContext& ctx) {
    float defaultRadius = m_bounds.height * 0.5f;
    float radius = GetProperty("cornerRadius").AsFloat(defaultRadius);
    D2D1_COLOR_F trackBg = GetProperty("trackColor").AsColor(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f));
    D2D1_COLOR_F fillBg = GetProperty("fillColor").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));

    // Draw Track
    ctx.FillRoundedRect(m_bounds, radius, trackBg);

    if (IsIndeterminate()) {
        ctx.PushClip(m_bounds);

        float W = m_bounds.width;
        float cycleDur = 2.0f;
        float animTime = m_animOffset;
        if (!UIElement::AreAnimationsEnabled()) {
            // Low performance mode: XP-style discrete frame-by-frame step animation (5 FPS step jumps)
            animTime = std::floor(m_animOffset * 5.0f) / 5.0f;
        }

        float tNorm = std::fmod(animTime, cycleDur) / cycleDur;
        if (tNorm < 0.0f) tNorm += 1.0f;

        // WinUI 3 Indicator 1 (Primary / Leading indicator)
        if (tNorm >= 0.0f && tNorm <= 0.75f) {
            float p1 = tNorm / 0.75f;
            float e1 = WinUI3EaseInOut(p1);
            float startX = m_bounds.x - 0.35f * W;
            float endX = m_bounds.x + 1.2f * W;
            float curX = startX + e1 * (endX - startX);
            float chunkW = W * (0.12f + 0.38f * std::sin(p1 * 3.14159265f));
            Rect r1(curX, m_bounds.y, chunkW, m_bounds.height);
            ctx.FillRoundedRect(r1, radius, fillBg);
        }

        // WinUI 3 Indicator 2 (Secondary / Trailing indicator)
        if (tNorm >= 0.35f && tNorm <= 1.0f) {
            float p2 = (tNorm - 0.35f) / 0.65f;
            float e2 = WinUI3EaseInOut(p2);
            float startX = m_bounds.x - 0.25f * W;
            float endX = m_bounds.x + 1.15f * W;
            float curX = startX + e2 * (endX - startX);
            float chunkW = W * (0.08f + 0.28f * std::sin(p2 * 3.14159265f));
            Rect r2(curX, m_bounds.y, chunkW, m_bounds.height);
            ctx.FillRoundedRect(r2, radius, fillBg);
        }

        ctx.PopClip();
    } else {
        float minVal = GetMinimum();
        float maxVal = GetMaximum();
        float val = std::clamp(m_displayValue, minVal, maxVal);
        float ratio = (maxVal > minVal) ? (val - minVal) / (maxVal - minVal) : 0.0f;

        float fillW = m_bounds.width * ratio;
        if (fillW > 0.0f) {
            Rect fillRect(m_bounds.x, m_bounds.y, fillW, m_bounds.height);
            ctx.FillRoundedRect(fillRect, radius, fillBg);
        }
    }
}

} // namespace CUI
