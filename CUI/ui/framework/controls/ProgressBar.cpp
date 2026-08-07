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
constexpr float kStripOverscan = 2.6f; // strip width = trackW * this
constexpr float kChunkFrac = 0.28f;    // fixed chunk width vs track
}

ProgressBar::ProgressBar() {
    SetFillColorToken(ThemeTokenId::AccentColor);
    SetTrackColorToken(ThemeTokenId::CardBorder);
    SetWidth(200.0f);
    SetHeight(3.0f);
    SetCornerRadius(1.5f);
    m_displayValue = GetValue();
    m_indicatorLayer.SetCacheable(true);
    // No Control hover chrome — ProgressBar does not paint hover fill.
    SetHoverBackgroundToken(ThemeTokenId::Unset);
    SetHoverBackground(D2D1::ColorF(0, 0, 0, 0));
    SetBackground(D2D1::ColorF(0, 0, 0, 0));
}

void ProgressBar::InvalidateIndicatorCache() {
    m_indicatorCacheValid = false;
    m_indicatorLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::SizeDirty);
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

bool ProgressBar::EnsureIndicatorStrip(
    GraphicsContext& ctx, float trackW, float trackH, float radius, D2D1_COLOR_F fill) {
    const float stripW = (std::max)(1.0f, trackW * kStripOverscan);
    const float stripH = (std::max)(1.0f, trackH);

    const bool colorChanged =
        std::abs(m_cachedFillColor.r - fill.r) > 0.002f
        || std::abs(m_cachedFillColor.g - fill.g) > 0.002f
        || std::abs(m_cachedFillColor.b - fill.b) > 0.002f
        || std::abs(m_cachedFillColor.a - fill.a) > 0.002f;

    if (m_indicatorCacheValid
        && std::abs(m_cachedStripW - stripW) < 0.5f
        && std::abs(m_cachedStripH - stripH) < 0.5f
        && !colorChanged
        && m_indicatorLayer.GetCacheBitmap()) {
        return true;
    }

    if (!ctx.PushLayerTarget(
            m_indicatorLayer,
            Size(stripW, stripH),
            Rect(0, 0, stripW, stripH),
            D2D1::ColorF(0, 0, 0, 0),
            true)) {
        return false;
    }

    // Fixed-width pill in the middle of the strip — compose slides this bitmap.
    const float chunkW = (std::max)(2.0f, trackW * kChunkFrac);
    const float chunkX = (stripW - chunkW) * 0.5f;
    ctx.FillRoundedRect(Rect(chunkX, 0.0f, chunkW, stripH), radius, fill);

    ctx.PopLayerTarget(m_indicatorLayer);
    m_indicatorLayer.Validate();
    m_indicatorCacheValid = true;
    m_cachedStripW = stripW;
    m_cachedStripH = stripH;
    m_cachedFillColor = fill;
    return m_indicatorLayer.GetCacheBitmap() != nullptr;
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

        const float W = m_bounds.width;
        const float cycleDur = 2.0f;
        float animTime = m_animOffset;
        if (!UIElement::AreAnimationsEnabled()) {
            animTime = std::floor(m_animOffset * 30.0f) / 30.0f;
        }
        float tNorm = std::fmod(animTime, cycleDur) / cycleDur;
        if (tNorm < 0.0f) {
            tNorm += 1.0f;
        }
        // Ease across the track: from fully left of clip to fully right.
        const float e = tNorm < 0.5f
            ? 4.0f * tNorm * tNorm * tNorm
            : 1.0f - std::pow(-2.0f * tNorm + 2.0f, 3.0f) / 2.0f;
        const float stripW = W * kStripOverscan;
        const float chunkW = W * kChunkFrac;
        const float startX = m_bounds.x - chunkW;
        const float endX = m_bounds.x + W;
        // Dest left for DrawLayer: strip's left edge so the centered pill tracks start→end.
        const float pillCenterStart = startX;
        const float pillCenterEnd = endX;
        const float pillCenter = pillCenterStart + e * (pillCenterEnd - pillCenterStart);
        m_prevSlideX = m_slideX;
        m_slideX = pillCenter - stripW * 0.5f;

        // Compose-only dirty: old∪new footprints (no content re-record).
        const Rect oldFoot(
            m_prevSlideX, m_bounds.y, stripW, m_bounds.height);
        const Rect newFoot(
            m_slideX, m_bounds.y, stripW, m_bounds.height);
        MarkRenderRectDirty(m_bounds.Union(oldFoot.Union(newFoot)).Inflate(2.0f));
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

void ProgressBar::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (!IsIndeterminate() || m_bounds.IsEmpty()) {
        UIElement::CollectSelfAnimationBounds(dirtyRect, hasDirty);
        return;
    }
    const float stripW = m_bounds.width * kStripOverscan;
    Rect foot(m_slideX, m_bounds.y, stripW, m_bounds.height);
    Rect unioned = m_bounds.Union(foot).Inflate(2.0f);
    dirtyRect = hasDirty ? dirtyRect.Union(unioned) : unioned;
    hasDirty = true;
}

void ProgressBar::OnRender(GraphicsContext& ctx) {
    if (IsIndeterminate() && !IsAnimationTicksRegistered()) {
        RequestAnimationTicks();
    }

    float radius = GetCornerRadius();
    if (radius < 0.0f) radius = m_bounds.height * 0.5f;
    D2D1_COLOR_F trackBg = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::CardBorder);
    D2D1_COLOR_F fillBg = ResolveThemeColor(GetFillColorToken(), ThemeTokenId::AccentColor);

    // Track is a cheap fill; indicator is a cached bitmap blit (compose slide).
    ctx.FillRoundedRect(m_bounds, radius, trackBg);

    if (IsIndeterminate()) {
        if (!EnsureIndicatorStrip(ctx, m_bounds.width, m_bounds.height, radius, fillBg)) {
            return;
        }
        ctx.PushClip(m_bounds);
        const float stripW = m_cachedStripW > 0.0f ? m_cachedStripW : m_bounds.width * kStripOverscan;
        const float stripH = m_cachedStripH > 0.0f ? m_cachedStripH : m_bounds.height;
        ctx.DrawLayer(m_indicatorLayer, Rect(m_slideX, m_bounds.y, stripW, stripH));
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
