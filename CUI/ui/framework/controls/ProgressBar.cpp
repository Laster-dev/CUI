#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ProgressBar.h"
#include "../animation/AnimationManager.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace CUI {

namespace {
float WinUI3EaseInOut(float t) {
    return t < 0.5f
        ? 4.0f * t * t * t
        : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}
} // namespace

ProgressBar::ProgressBar() {
    SetFillColorToken(ThemeTokenId::AccentColor);
    SetTrackColorToken(ThemeTokenId::CardBorder);
    SetWidth(200.0f);
    SetHeight(3.0f);
    SetCornerRadius(1.5f);
    m_displayValue = GetValue();
    // No Control hover chrome — ProgressBar does not paint hover fill.
    SetHoverBackgroundToken(ThemeTokenId::Unset);
    SetHoverBackground(D2D1::ColorF(0, 0, 0, 0));
    SetBackground(D2D1::ColorF(0, 0, 0, 0));
}

std::vector<PropertyMeta> ProgressBar::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "value", "进度数值 (Value)", "进度配置", "number" });
    metas.push_back({ "minimum", "最小值 (Minimum)", "进度配置", "number" });
    metas.push_back({ "maximum", "最大值 (Maximum)", "进度配置", "number" });
    metas.push_back({ "isIndeterminate", "不确定模式 (IsIndeterminate)", "进度配置", "bool" });
    return metas;
}

Size ProgressBar::Measure(Size availableSize) {
    (void)availableSize;
    float expW = GetWidth(); if (expW < 0) expW = 200.0f;
    float expH = GetHeight(); if (expH < 0) expH = 3.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

bool ProgressBar::OnAnimationTick() {
    if (AnimationManager* mgr = AnimationManager::Current()) {
        if (!mgr->IsInLiveTree(this)) {
            return false;
        }
    }

    const float deltaSeconds = UIElement::GetAnimationDeltaSeconds();
    m_lastTickTime = std::chrono::steady_clock::now();

    if (IsIndeterminate()) {
        if (m_visibility != Visibility::Visible || m_bounds.IsEmpty()) {
            return false;
        }
        const float speed = 1.0f;
        m_animOffset += speed * deltaSeconds;
        if (m_animOffset > 10000.0f) {
            m_animOffset = std::fmod(m_animOffset, 2.0f);
        }
        MarkRenderRectDirty(m_bounds);
        RequestAnimationTicks();
        return true;
    }

    if (!UIElement::AreAnimationsEnabled()) {
        m_displayValue = GetValue();
        return false;
    }

    float target = GetValue();
    float delta = target - m_displayValue;
    if (std::abs(delta) <= 0.01f) {
        m_displayValue = target;
        return false;
    }
    float smoothing = 1.0f - std::exp(-12.0f * deltaSeconds);
    m_displayValue += delta * smoothing;
    MarkRenderRectDirty(m_bounds);
    RequestAnimationTicks();
    return true;
}

bool ProgressBar::HasSelfAnimation() const {
    if (m_visibility != Visibility::Visible || m_bounds.IsEmpty()) {
        return false;
    }
    return IsIndeterminate()
        || (UIElement::AreAnimationsEnabled() && std::abs(GetValue() - m_displayValue) > 0.01f);
}

void ProgressBar::OnRender(GraphicsContext& ctx) {
    // Kick the pump when this bar is actually painted (live tree / page visible).
    // SetValue before attach often fails to keep ticks registered.
    if (IsIndeterminate() && !IsAnimationTicksRegistered()) {
        RequestAnimationTicks();
    } else if (!IsIndeterminate()
        && UIElement::AreAnimationsEnabled()
        && std::abs(GetValue() - m_displayValue) > 0.01f
        && !IsAnimationTicksRegistered()) {
        RequestAnimationTicks();
    }

    float radius = GetCornerRadius();
    if (radius < 0.0f) radius = m_bounds.height * 0.5f;
    D2D1_COLOR_F trackBg = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::CardBorder);
    D2D1_COLOR_F fillBg = ResolveThemeColor(GetFillColorToken(), ThemeTokenId::AccentColor);

    ctx.FillRoundedRect(m_bounds, radius, trackBg);

    if (IsIndeterminate()) {
        ctx.PushClip(m_bounds);

        const float W = m_bounds.width;
        constexpr float cycleDur = 2.0f;
        float animTime = m_animOffset;
        if (!UIElement::AreAnimationsEnabled()) {
            // Low-perf: discrete step jumps (~8 FPS cadence).
            animTime = std::floor(m_animOffset * 8.0f) / 8.0f;
        }

        float tNorm = std::fmod(animTime, cycleDur) / cycleDur;
        if (tNorm < 0.0f) {
            tNorm += 1.0f;
        }

        // WinUI 3 Indicator 1 (Primary / Leading indicator)
        if (tNorm >= 0.0f && tNorm <= 0.75f) {
            const float p1 = tNorm / 0.75f;
            const float e1 = WinUI3EaseInOut(p1);
            const float startX = m_bounds.x - 0.35f * W;
            const float endX = m_bounds.x + 1.2f * W;
            const float curX = startX + e1 * (endX - startX);
            const float chunkW = W * (0.12f + 0.38f * std::sin(p1 * 3.14159265f));
            ctx.FillRoundedRect(Rect(curX, m_bounds.y, chunkW, m_bounds.height), radius, fillBg);
        }

        // WinUI 3 Indicator 2 (Secondary / Trailing indicator)
        if (tNorm >= 0.35f && tNorm <= 1.0f) {
            const float p2 = (tNorm - 0.35f) / 0.65f;
            const float e2 = WinUI3EaseInOut(p2);
            const float startX = m_bounds.x - 0.25f * W;
            const float endX = m_bounds.x + 1.15f * W;
            const float curX = startX + e2 * (endX - startX);
            const float chunkW = W * (0.08f + 0.28f * std::sin(p2 * 3.14159265f));
            ctx.FillRoundedRect(Rect(curX, m_bounds.y, chunkW, m_bounds.height), radius, fillBg);
        }

        ctx.PopClip();
    } else {
        // If value was set while detached / before first paint, snap so the bar shows
        // immediately instead of staying empty until a click starts the anim pump.
        if (!IsAnimationTicksRegistered()
            && std::abs(GetValue() - m_displayValue) > 0.01f) {
            if (!UIElement::AreAnimationsEnabled()) {
                m_displayValue = GetValue();
            }
        }

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
