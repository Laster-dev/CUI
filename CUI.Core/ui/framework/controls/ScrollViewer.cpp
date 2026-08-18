#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ScrollViewer.h"
#include "MiddleClickAutoscroll.h"
#include "../animation/FrameScheduler.h"
#include "../render/CompositionContext.h"
#include "../style/ThemeManager.h"
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <windows.h>

namespace CUI {

namespace {
float GetChromiumWheelStep(float viewportHeight) {
    UINT lines = 3;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    if (lines == WHEEL_PAGESCROLL) {
        return (std::max)(40.0f, viewportHeight);
    }
    return (std::max)(1u, lines) * 40.0f;
}

float GetVisualBottom(UIElement* element) {
    if (!element || element->GetVisibility() != Visibility::Visible) {
        return 0.0f;
    }

    Rect bounds = element->GetBounds();
    float bottom = bounds.y + bounds.height;

    if (element->ShouldClipToBounds()) {
        return bottom;
    }

    for (const auto& child : element->GetChildren()) {
        bottom = (std::max)(bottom, GetVisualBottom(child.get()));
    }
    return bottom;
}

void RenderVisibleSubtree(UIElement* element, GraphicsContext& ctx, const Rect& visibleRect) {
    if (!element) {
        return;
    }

    const Visibility visibility = element->GetVisibility();
    if (visibility != Visibility::Visible) {
        return;
    }

    const Rect bounds = element->GetBounds();
    const bool clipToBounds = element->ShouldClipToBounds();
    if (clipToBounds && !bounds.IsEmpty() && !bounds.Intersects(visibleRect)) {
        return;
    }

    element->Render(ctx);
}
}

ScrollViewer::ScrollViewer() {
    SetTrackColorToken(ThemeTokenId::CardBorder);
    SetThumbColorToken(ThemeTokenId::TextMuted);
    SetBackground(D2D1::ColorF(0, 0, 0, 0));
    QueryPerformanceFrequency(&m_qpcFreq);
    m_scrollAnimator.Reset(0.0f);
    GetRenderNode().GetLayer().SetCacheable(true);
    m_contentLayer.SetCacheable(true);
    // Do NOT hook OnPropertyIdChanged → MarkContentLayerDirty here.
    // UIElement already MarkRenderContentDirties on NotifyFieldChanged; a second
    // StructureDirty path forced FULL_RERASTER of the whole menu/PropertyGrid
    // bitmap on every token/opacity write (identical hitch to the old scroll bug).
}

void ScrollViewer::SetScrollOffsetY(float offset) {
    float previousOffset = m_offsetY;
    StopSmoothScroll();
    m_offsetY = offset;
    ClampOffset();
    m_scrollAnimator.JumpTo(m_offsetY);
    PositionChildren();
    if (std::abs(previousOffset - m_offsetY) > 0.01f) {
        MarkScrollVisualDirty(previousOffset);
    }
}

void ScrollViewer::StopSmoothScroll() {
    m_scrollAnimator.JumpTo(m_offsetY);
    m_lastAnimQpc = 0;
}

float ScrollViewer::GetMaxScroll() const {
    return (std::max)(0.0f, m_contentHeight - m_bounds.height);
}

float ScrollViewer::GetScrollbarReserve() const {
    if (m_overlayScrollbar) {
        return 0.0f;
    }
    if (m_contentHeight > m_bounds.height && m_bounds.height > 0.0f) {
        return kScrollbarInset + kScrollbarWidth;
    }
    return 0.0f;
}

Rect ScrollViewer::GetScrollbarTrackRect() const {
    float trackX = m_bounds.x + m_bounds.width - kScrollbarInset - kScrollbarWidth;
    return Rect(trackX, m_bounds.y, kScrollbarWidth, m_bounds.height);
}

Rect ScrollViewer::GetScrollbarThumbRect() const {
    Rect track = GetScrollbarTrackRect();
    float maxScroll = GetMaxScroll();
    if (maxScroll <= 0.0f || m_bounds.height <= 0.0f || m_contentHeight <= 0.0f) {
        return Rect(track.x, track.y, track.width, 0.0f);
    }

    float thumbHeight = (m_bounds.height / m_contentHeight) * track.height;
    if (thumbHeight < 24.0f) thumbHeight = 24.0f;
    if (thumbHeight > track.height) thumbHeight = track.height;

    float scrollRatio = m_offsetY / maxScroll;
    float thumbY = track.y + scrollRatio * (track.height - thumbHeight);
    return Rect(track.x, thumbY, track.width, thumbHeight);
}

void ScrollViewer::ClampOffset() {
    m_offsetY = std::clamp(m_offsetY, 0.0f, GetMaxScroll());
    m_scrollAnimator.ClampTo(0.0f, GetMaxScroll());
}

double ScrollViewer::SecondsSinceLastTick() {
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);

    if (m_lastAnimQpc == 0 || m_qpcFreq.QuadPart <= 0) {
        m_lastAnimQpc = now.QuadPart;
        return 1.0 / 120.0;
    }

    double dt = static_cast<double>(now.QuadPart - m_lastAnimQpc) / static_cast<double>(m_qpcFreq.QuadPart);
    m_lastAnimQpc = now.QuadPart;

    if (dt < 0.0005) dt = 0.0005;
    if (dt > 0.050) dt = 0.050;
    return dt;
}

float ScrollViewer::MeasureContentHeight(float contentWidth) {
    Thickness padding = GetPadding();
    float height = 0.0f;
    Size avail(std::max(0.0f, contentWidth), 100000.0f);
    for (auto& child : GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;
        Size dSize = child->Measure(avail);
        height = std::max(height, dSize.height);
    }
    return padding.top + height + padding.bottom + kContentBottomPad;
}

void ScrollViewer::RefreshContentMetrics(float viewportWidth, float viewportHeight) {
    Thickness padding = GetPadding();
    float innerWidth = std::max(0.0f, viewportWidth - padding.left - padding.right);

    // Measure without mutating "last measured" width mid-pass. The old two-step
    // path wrote m_measuredContentWidth=full then =full-scrollbar, so the next
    // frame always looked like a metrics change and MarkContentLayerDirty'd the
    // menu/PropertyGrid every frame (732↔758 oscillation).
    const float prevMeasuredHeight = m_measuredContentHeight;
    const float prevWidth = m_measuredContentWidth;
    const float prevContentHeight = m_contentHeight;

    float usedWidth = innerWidth;
    float measuredHeight = MeasureContentHeight(innerWidth);
    // Overlay scrollbars do not steal content width — subtracting them here made
    // Arrange (reserve=0) disagree with Measure and MarkContentLayerDirty every
    // Relayout while the menu was animating ripples.
    if (!m_overlayScrollbar && measuredHeight > viewportHeight) {
        usedWidth = std::max(0.0f, innerWidth - kScrollbarInset - kScrollbarWidth);
        measuredHeight = MeasureContentHeight(usedWidth);
    }

    m_measuredContentHeight = measuredHeight;
    m_measuredContentWidth = usedWidth;

    // PositionChildren may discover a taller visual subtree (PropertyGrid paints
    // past DesiredSize). Keep that floor so Measure→Position→Measure does not
    // oscillate 580↔758 and MarkContentLayerDirty every frame.
    if (measuredHeight + 0.5f < m_visualContentHeight
        && std::abs(prevMeasuredHeight - measuredHeight) > 0.5f) {
        // Real measured shrink (collapse / page swap) — drop the visual floor.
        m_visualContentHeight = measuredHeight;
    }
    m_contentHeight = (std::max)(measuredHeight, m_visualContentHeight);

    const bool measuredChg =
        std::abs(prevMeasuredHeight - m_measuredContentHeight) > 0.5f
        || std::abs(prevWidth - m_measuredContentWidth) > 0.5f;
    const bool cacheGrew =
        m_contentHeight > prevContentHeight + 0.5f;
    if (measuredChg || cacheGrew) {
        MarkContentLayerDirty();
    }
    ClampOffset();
}

void ScrollViewer::PositionChildren() {
    Thickness padding = GetPadding();
    float innerWidth = std::max(0.0f, m_bounds.width - padding.left - padding.right);
    float reserve = GetScrollbarReserve();
    float childWidth = std::max(0.0f, innerWidth - reserve);
    float viewportContentHeight = std::max(0.0f, m_bounds.height - padding.top - padding.bottom);

    // Scrolling changes only child positions. Do not Measure here: thumb dragging
    // and inertial animation run at frame rate.
    float naturalHeight = 0.0f;
    for (auto& child : GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;
        naturalHeight = std::max(naturalHeight, child->GetDesiredSize().height);
    }
    // Do NOT stretch the content host to the viewport when content is shorter.
    // Stretching made GetVisualBottom == viewport bottom → contentH ≈ viewport →
    // GetMaxScroll() == 0 → wheel/drag looked dead ("滚动没反应").
    float childHeight = (naturalHeight > 0.5f) ? naturalHeight : viewportContentHeight;

    for (auto& child : GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;
        Rect childRect(
            m_bounds.x + padding.left,
            m_bounds.y + padding.top - m_offsetY,
            childWidth,
            childHeight
        );
        child->Arrange(childRect);
    }

    float visualBottom = 0.0f;
    for (auto& child : GetChildren()) {
        if (child->GetVisibility() == Visibility::Collapsed) continue;
        visualBottom = (std::max)(visualBottom, GetVisualBottom(child.get()));
    }

    if (visualBottom > 0.0f) {
        float contentTop = m_bounds.y + padding.top - m_offsetY;
        float visualContentHeight = visualBottom - contentTop + padding.bottom + kContentBottomPad;
        // Ignore visual "growth" that is only the old stretch-to-viewport artifact.
        if (visualContentHeight > m_measuredContentHeight + 0.5f
            && visualContentHeight > m_visualContentHeight + 0.5f) {
            m_visualContentHeight = visualContentHeight;
        }
        if (visualContentHeight > m_contentHeight + 0.5f
            && visualContentHeight > m_measuredContentHeight + 0.5f) {
            // PropertyGrid (and similar) may paint past DesiredSize — grow scroll
            // range to the real visual subtree, but never from viewport stretch.
            m_contentHeight = visualContentHeight;
            ClampOffset();
        }
    }
}

Rect ScrollViewer::GetViewportRect() const {
    return m_bounds;
}

Rect ScrollViewer::GetContentViewportRect() const {
    Thickness padding = GetPadding();
    float reserve = GetScrollbarReserve();
    return Rect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        (std::max)(0.0f, m_bounds.width - padding.left - padding.right - reserve),
        (std::max)(0.0f, m_bounds.height - padding.top - padding.bottom)
    );
}

void ScrollViewer::MarkContentLayerDirty() {
    // StructureDirty discards m_valid → guaranteed FULL_RERASTER (expand/collapse,
    // metrics). Soft ContentDirty alone is reserved for strip-patch ripples.
    m_contentLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty);
    m_pendingViewportScrollPatch = false;
    m_pendingViewportPatchDeltaY = 0.0f;
    Rect viewport = GetContentViewportRect();
    if (!viewport.IsEmpty()) {
        m_contentLayerDirty.AddRect(viewport.Inflate(2.0f));
        UIElement::MarkRenderRectDirty(viewport.Inflate(2.0f));
    }
}

void ScrollViewer::InvalidateContentLayout() {
    m_visualContentHeight = 0.0f;
    m_measuredContentHeight = 0.0f;
    m_measuredContentWidth = -1.0f;
    for (auto& child : GetChildren()) {
        if (child) {
            child->InvalidateMeasure();
        }
    }
    InvalidateMeasure();
    MarkContentLayerDirty();
}

void ScrollViewer::MarkContentLayerRectDirty(const Rect& rect) {
    if (rect.IsEmpty()) {
        return;
    }
    m_contentLayer.Invalidate(RenderLayer::ContentDirty);
    m_contentLayerDirty.AddRect(rect);
    UIElement::MarkRenderRectDirty(rect);
}

void ScrollViewer::UpdateContentLayerState() {
    Rect contentViewport = GetContentViewportRect();
    m_contentViewportRect = contentViewport;
    m_contentLayer.SetBounds(contentViewport);
    m_contentLayer.SetTranslation(0.0f, -m_offsetY);
    m_contentLayerOffsetY = m_offsetY;
}

bool ScrollViewer::ShouldRenderFullContentLayer(const GraphicsContext& ctx) const {
    if (!m_contentLayer.IsValid()) {
        return true;
    }
    // Hard structural changes require a full bitmap rebuild.
    constexpr unsigned kHardMask =
        RenderLayer::SizeDirty
        | RenderLayer::StructureDirty
        | RenderLayer::ClipDirty;
    if ((m_contentLayer.GetDirtyFlags() & kHardMask) != 0) {
        return true;
    }
    if (!ctx.IntersectsPaintBounds(m_contentViewportRect)) {
        return false;
    }
    // Soft ContentDirty with localized rects → strip patch (ripple/hover).
    // Only a dirty covering most of the viewport/cache forces FULL.
    if ((m_contentLayer.GetDirtyFlags() & RenderLayer::ContentDirty) != 0) {
        const size_t n = m_contentLayerDirty.GetRectCount();
        // Collect used to clear dirty rects before paint, leaving ContentDirty with
        // n==0 — that must NOT force FULL (it made PropertyGrid rebuild every frame).
        if (n == 0) {
            return false;
        }
        const float vpArea = (std::max)(1.0f, m_contentViewportRect.width * m_contentViewportRect.height);
        Rect unionRect = m_contentLayerDirty.GetBounds();
        const float dirtyArea = (std::max)(0.0f, unionRect.width * unionRect.height);
        // Multiple nav ripples must STRIP the union — never FULL the whole menu.
        if (dirtyArea >= vpArea * 0.85f) {
            return true;
        }
        return false; // strip patch
    }
    return false;
}

void ScrollViewer::MarkScrollVisualDirty(float previousOffset) {
    if (m_bounds.IsEmpty()) {
        return;
    }

    // Scroll PositionChildren already moved descendants (transform dirties). Swallow
    // those before Collect or the HWND dirty region becomes the full document height.
    // Do NOT swallow in PositionChildren itself — expand/collapse layout needs those
    // dirties (or MeasureContentHeight MarkContentLayerDirty) to re-raster the menu.
    SwallowDescendantRenderDirties();

    Rect contentViewport = GetContentViewportRect();
    const bool cacheFullContent = m_contentHeight > 0.0f && m_contentHeight <= kMaxFullContentCacheHeight;
    if (cacheFullContent) {
        // Full-content bitmap already holds the whole scroll range — scrolling is a
        // sourceRect blit only. Do NOT call MarkRenderRectDirty (override ContentDirties
        // the content layer) or we re-rasterize every child every frame.
        if (!contentViewport.IsEmpty()) {
            UIElement::MarkRenderRectDirty(contentViewport.Inflate(2.0f));
        }
        m_pendingViewportScrollPatch = false;
        m_pendingViewportPatchDeltaY = 0.0f;
    } else if (!contentViewport.IsEmpty()) {
        // Tall content: shift/patch the viewport cache. TransformDirty alone must not
        // ContentDirty the layer or canPatchViewportCache never sees a valid bitmap.
        m_contentLayer.Invalidate(RenderLayer::TransformDirty);
        m_pendingViewportScrollPatch = true;
        m_pendingViewportPatchDeltaY = m_offsetY - previousOffset;
        UIElement::MarkRenderRectDirty(contentViewport.Inflate(2.0f));
    }

    if (m_contentHeight > m_bounds.height) {
        // Scrollbar chrome is drawn outside the content layer — scene dirty only.
        UIElement::MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));

        float maxScroll = GetMaxScroll();
        if (maxScroll > 0.0f) {
            Rect track = GetScrollbarTrackRect();
            float thumbHeight = (m_bounds.height / m_contentHeight) * track.height;
            thumbHeight = std::clamp(thumbHeight, 24.0f, track.height);
            float previousRatio = std::clamp(previousOffset / maxScroll, 0.0f, 1.0f);
            float previousThumbY = track.y + previousRatio * (track.height - thumbHeight);
            UIElement::MarkRenderRectDirty(Rect(track.x, previousThumbY, track.width, thumbHeight).Inflate(2.0f));
            UIElement::MarkRenderRectDirty(GetScrollbarThumbRect().Inflate(2.0f));
        }
    }
}

Size ScrollViewer::Measure(Size availableSize) {
    Thickness margin = GetMargin();
    Thickness padding = GetPadding();

    float expW = GetWidth();
    float expH = GetHeight();

    // Use the control's own width when explicit — NOT the parent's full available width.
    // Measuring with the window width was underestimating wrapped content for a 320px panel.
    float finalW = (expW >= 0.0f) ? expW : availableSize.width;
    float finalH = (expH >= 0.0f) ? expH : availableSize.height;
    RefreshContentMetrics(finalW - margin.left - margin.right, finalH - margin.top - margin.bottom);

    m_desiredSize = Size(finalW, finalH);
    return m_desiredSize;
}

void ScrollViewer::Arrange(Rect finalRect) {
    SetBounds(finalRect);
    Thickness padding = GetPadding();
    float reserve = GetScrollbarReserve();
    float contentWidth = std::max(
        0.0f,
        finalRect.width - padding.left - padding.right - reserve
    );
    if (std::abs(contentWidth - m_measuredContentWidth) > 0.5f) {
        RefreshContentMetrics(finalRect.width, finalRect.height);
    }
    PositionChildren();
    UpdateContentLayerState();
}

void ScrollViewer::Render(GraphicsContext& ctx) {
    if (GetVisibility() != Visibility::Visible) return;

    ctx.PushClip(m_bounds);
    OnRender(ctx);

    // Scrollbar show/hide dirties only the track. Skip content-layer blit when the
    // paint bounds sit inside that strip — otherwise every auto-hide tick still
    // sampled a tall content bitmap through the overlay track.
    const Rect paintBounds = ctx.GetPaintBounds();
    const Rect track = GetScrollbarTrackRect().Inflate(4.0f);
    const bool chromeOnly =
        !paintBounds.IsEmpty()
        && m_contentHeight > m_bounds.height
        && paintBounds.width <= track.width + 8.0f
        && paintBounds.x >= track.x - 4.0f
        && (paintBounds.x + paintBounds.width) <= (track.x + track.width + 4.0f)
        && m_contentLayer.IsValid()
        && m_contentLayer.GetCacheBitmap() != nullptr
        && (m_contentLayer.GetDirtyFlags()
            & (RenderLayer::SizeDirty | RenderLayer::StructureDirty | RenderLayer::ClipDirty
                | RenderLayer::ContentDirty)) == 0;
    if (!chromeOnly) {
        RenderContentImmediate(ctx);
    }
    RenderScrollChrome(ctx);
    if (m_middleScrollActive) {
        const bool dark = ThemeManager::Instance().GetThemeMode() == ThemeMode::Dark;
        D2D1_COLOR_F fill = dark
            ? D2D1::ColorF(0.18f, 0.18f, 0.20f, 0.92f)
            : D2D1::ColorF(0.96f, 0.96f, 0.97f, 0.95f);
        D2D1_COLOR_F stroke = dark
            ? D2D1::ColorF(0.70f, 0.70f, 0.74f, 0.95f)
            : D2D1::ColorF(0.35f, 0.35f, 0.40f, 0.90f);
        D2D1_COLOR_F arrow = dark
            ? D2D1::ColorF(0.92f, 0.92f, 0.95f, 1.0f)
            : D2D1::ColorF(0.20f, 0.20f, 0.24f, 1.0f);
        MiddleClickAutoscroll::PaintOriginIndicator(
            ctx, m_middleOrigin, false, GetMaxScroll() > 0.0f, fill, stroke, arrow);
    }

    ctx.PopClip();
}

void ScrollViewer::OnRender(GraphicsContext& ctx) {
    UIElement::OnRender(ctx);
}

void ScrollViewer::RenderContentImmediate(GraphicsContext& ctx) {
    if (m_contentViewportRect.IsEmpty()) {
        return;
    }

    ctx.PushClip(m_contentViewportRect);
    for (auto& child : GetChildren()) {
        RenderVisibleSubtree(child.get(), ctx, m_contentViewportRect);
    }
    ctx.PopClip();

    m_contentLayer.ResetCache();
    m_contentLayerDirty.Clear();
    m_contentLayer.Validate();
    m_contentLayer.ClearDirtyFlags(
        RenderLayer::ContentDirty
        | RenderLayer::TransformDirty
        | RenderLayer::OpacityDirty
        | RenderLayer::SizeDirty
        | RenderLayer::StructureDirty
        | RenderLayer::ClipDirty);
    m_contentLayerCachesFullContent = false;
    m_pendingViewportScrollPatch = false;
    m_pendingViewportPatchDeltaY = 0.0f;
    m_contentLayerOffsetY = m_offsetY;
    m_contentLayer.SetTranslation(0.0f, -m_offsetY);
}

void ScrollViewer::RenderContentLayer(GraphicsContext& ctx) {
    if (m_contentViewportRect.IsEmpty()) {
        return;
    }

    const bool cacheFullContent = m_contentHeight > 0.0f && m_contentHeight <= kMaxFullContentCacheHeight;
    const float cacheHeight = cacheFullContent
        ? (std::max)(m_contentViewportRect.height, m_contentHeight)
        : m_contentViewportRect.height;
    const Size cacheSize(m_contentViewportRect.width, cacheHeight);
    // EnsureLayerCache stores ceil(dips) — compare against the same, or every frame
    // looks like a size change (e.g. 3437.5 vs stored 3438) and forces FULL_RERASTER.
    const Size cacheSizeCeiled(
        (std::max)(1.0f, std::ceil(cacheSize.width)),
        (std::max)(1.0f, std::ceil(cacheSize.height)));

    const bool modeChanged = m_contentLayerCachesFullContent != cacheFullContent;
    const bool sizeChanged =
        std::abs(m_contentLayer.GetCacheSurfaceSize().width - cacheSizeCeiled.width) > 0.5f
        || std::abs(m_contentLayer.GetCacheSurfaceSize().height - cacheSizeCeiled.height) > 0.5f;
    const bool canPatchViewportCache =
        !cacheFullContent
        && !modeChanged
        && !sizeChanged
        && m_contentLayer.IsValid()
        && m_contentLayer.GetCacheBitmap() != nullptr
        && m_pendingViewportScrollPatch
        && std::abs(m_pendingViewportPatchDeltaY) > 0.01f
        && std::abs(m_pendingViewportPatchDeltaY) < m_contentViewportRect.height
        // Don't shift-patch over pending ripple/hover — that Validate()'d away
        // ContentDirty and left stuck pixels in the viewport cache.
        && (m_contentLayer.GetDirtyFlags() & RenderLayer::ContentDirty) == 0;
    constexpr unsigned kHardMask =
        RenderLayer::SizeDirty | RenderLayer::StructureDirty | RenderLayer::ClipDirty;
    const bool canStripPatchContent =
        !canPatchViewportCache
        && !modeChanged
        && !sizeChanged
        && m_contentLayer.IsValid()
        && m_contentLayer.GetCacheBitmap() != nullptr
        && (m_contentLayer.GetDirtyFlags() & RenderLayer::ContentDirty) != 0
        && (m_contentLayer.GetDirtyFlags() & kHardMask) == 0
        && m_contentLayerDirty.GetRectCount() >= 1
        && !ShouldRenderFullContentLayer(ctx);
    const bool needsRerender = modeChanged || sizeChanged || ShouldRenderFullContentLayer(ctx);

    if (auto* composition = ctx.GetCompositionContext()) {
        if (canPatchViewportCache || canStripPatchContent) {
            composition->CountLayerCacheHit();
            composition->CountLayerCacheReuse();
        } else if (needsRerender) {
            composition->CountLayerCacheRerender();
            if (modeChanged || sizeChanged || !m_contentLayer.GetCacheBitmap()) {
                composition->CountLayerCacheMiss();
            }
        } else {
            composition->CountLayerCacheHit();
            composition->CountLayerCacheReuse();
        }
    }

    Rect visibleWorldRect = m_contentViewportRect;
    Rect fullContentWorldRect(
        m_contentViewportRect.x,
        m_contentViewportRect.y - m_offsetY,
        m_contentViewportRect.width,
        cacheHeight
    );

    if (canPatchViewportCache && ctx.EnsureLayerScratch(m_contentLayer)) {
        // Snapshot BEFORE PushLayerTarget: once the cache bitmap is the D2D target,
        // Clear+CopyFromBitmap races and produces torn/jittery scroll frames.
        ID2D1Bitmap1* cacheBmp = m_contentLayer.GetCacheBitmap();
        ID2D1Bitmap1* scratchBmp = m_contentLayer.GetScratchBitmap();
        if (cacheBmp && scratchBmp) {
            scratchBmp->CopyFromBitmap(nullptr, cacheBmp, nullptr);
        }

        if (ctx.PushLayerTarget(
                m_contentLayer,
                cacheSize,
                visibleWorldRect,
                D2D1::ColorF(0, 0, 0, 0),
                false)) {
            auto* d2d = ctx.GetD2DContext();
            const float deltaY = m_pendingViewportPatchDeltaY;

            if (scratchBmp) {
                Rect shiftedDest(0.0f, -deltaY, cacheSize.width, cacheSize.height);
                d2d->DrawBitmap(
                    scratchBmp,
                    shiftedDest.ToD2D(),
                    1.0f,
                    D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                    nullptr);
            }

            Rect exposedRect;
            if (deltaY > 0.0f) {
                exposedRect = Rect(0.0f, cacheSize.height - deltaY, cacheSize.width, deltaY);
            } else {
                exposedRect = Rect(0.0f, 0.0f, cacheSize.width, -deltaY);
            }

            if (!exposedRect.IsEmpty()) {
                ctx.ClearRect(exposedRect);
                ctx.PushClip(exposedRect);
                D2D1_MATRIX_3X2_F oldTransform{};
                d2d->GetTransform(&oldTransform);
                const float contentTop = m_contentViewportRect.y - m_offsetY;
                d2d->SetTransform(D2D1::Matrix3x2F::Translation(-m_contentViewportRect.x, -contentTop));
                for (auto& child : GetChildren()) {
                    RenderVisibleSubtree(child.get(), ctx, Rect(
                        m_contentViewportRect.x,
                        m_contentViewportRect.y + exposedRect.y,
                        m_contentViewportRect.width,
                        exposedRect.height
                    ));
                }
                d2d->SetTransform(oldTransform);
                ctx.PopClip();
            }

            ctx.PopLayerTarget(m_contentLayer);
            m_contentLayer.Validate();
            m_contentLayerDirty.Clear();
            m_contentLayerCachesFullContent = false;
            m_pendingViewportScrollPatch = false;
            m_pendingViewportPatchDeltaY = 0.0f;
        }
    } else if (canStripPatchContent && ctx.PushLayerTarget(
        m_contentLayer,
        cacheSize,
        cacheFullContent ? fullContentWorldRect : visibleWorldRect,
        D2D1::ColorF(0, 0, 0, 0),
        false)) {
        // Localized content update (ripple/hover) into the existing cache bitmap.
        // Clear/clip in WORLD space while the world→layer transform is active —
        // converting to layer-local first double-transformed the rects and missed
        // the item (stuck nav ripples / "animations don't work").
        auto* d2d = ctx.GetD2DContext();
        const float contentTop = m_contentViewportRect.y - (cacheFullContent ? m_offsetY : 0.0f);
        D2D1_MATRIX_3X2_F oldTransform{};
        d2d->GetTransform(&oldTransform);
        d2d->SetTransform(D2D1::Matrix3x2F::Translation(-m_contentViewportRect.x, -contentTop));

        for (const Rect& worldDirty : m_contentLayerDirty.GetRects()) {
            Rect clipped = worldDirty;
            // Intersect with the world rect covered by the cache.
            const Rect cacheWorld = cacheFullContent ? fullContentWorldRect : visibleWorldRect;
            const float x0 = (std::max)(clipped.x, cacheWorld.x);
            const float y0 = (std::max)(clipped.y, cacheWorld.y);
            const float x1 = (std::min)(clipped.x + clipped.width, cacheWorld.x + cacheWorld.width);
            const float y1 = (std::min)(clipped.y + clipped.height, cacheWorld.y + cacheWorld.height);
            if (x1 <= x0 || y1 <= y0) {
                continue;
            }
            clipped = Rect(x0, y0, x1 - x0, y1 - y0);
            ctx.ClearRect(clipped);
            ctx.PushClip(clipped);
            for (auto& child : GetChildren()) {
                RenderVisibleSubtree(child.get(), ctx, clipped);
            }
            ctx.PopClip();
        }

        d2d->SetTransform(oldTransform);
        ctx.PopLayerTarget(m_contentLayer);
        m_contentLayer.Validate();
        m_contentLayerDirty.Clear();
        m_contentLayerCachesFullContent = cacheFullContent;
        m_pendingViewportScrollPatch = false;
        m_pendingViewportPatchDeltaY = 0.0f;
    } else if (needsRerender && ctx.PushLayerTarget(
        m_contentLayer,
        cacheSize,
        cacheFullContent ? fullContentWorldRect : visibleWorldRect,
        D2D1::ColorF(0, 0, 0, 0))) {
        const float contentTop = m_contentViewportRect.y - (cacheFullContent ? m_offsetY : 0.0f);
        auto* d2d = ctx.GetD2DContext();
        D2D1_MATRIX_3X2_F oldTransform{};
        d2d->GetTransform(&oldTransform);
        d2d->SetTransform(D2D1::Matrix3x2F::Translation(-m_contentViewportRect.x, -contentTop));

        for (auto& child : GetChildren()) {
            RenderVisibleSubtree(child.get(), ctx, cacheFullContent
                ? fullContentWorldRect
                : m_contentViewportRect);
        }

        d2d->SetTransform(oldTransform);
        ctx.PopLayerTarget(m_contentLayer);
        m_contentLayer.Validate();
        m_contentLayerDirty.Clear();
        m_contentLayerCachesFullContent = cacheFullContent;
        m_pendingViewportScrollPatch = false;
        m_pendingViewportPatchDeltaY = 0.0f;
    }

    Rect sourceRect(
        0.0f,
        cacheFullContent ? m_offsetY : 0.0f,
        m_contentViewportRect.width,
        m_contentViewportRect.height
    );
    ctx.PushClip(m_contentViewportRect);
    ctx.DrawLayer(m_contentLayer, m_contentViewportRect, &sourceRect);
    ctx.PopClip();

    // If we only blit (scroll) or Collect left ContentDirty with emptied rects,
    // clear soft flags so the next frame does not FULL_RERASTER forever.
    if (!needsRerender && !canStripPatchContent && !canPatchViewportCache) {
        m_contentLayer.ClearDirtyFlags(
            RenderLayer::ContentDirty
            | RenderLayer::TransformDirty
            | RenderLayer::OpacityDirty);
        m_contentLayerDirty.Clear();
    }

    m_contentLayerOffsetY = m_offsetY;
    m_contentLayer.SetTranslation(0.0f, -m_offsetY);
}

void ScrollViewer::RenderScrollChrome(GraphicsContext& ctx) {
    if (m_contentHeight <= m_bounds.height || m_bounds.height <= 0.0f) {
        return;
    }
    const float visibility = m_scrollbarAutoHide.Opacity();
    if (visibility <= 0.01f) {
        return;
    }

    Rect track = GetScrollbarTrackRect();
    Rect thumb = GetScrollbarThumbRect();

    D2D1_COLOR_F trackBase = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::CardBorder);
    D2D1_COLOR_F thumbBase = ResolveThemeColor(GetThumbColorToken(), ThemeTokenId::TextMuted);
    float trackAlpha = (m_scrollbarHovered || m_isDraggingThumb ? 0.18f : 0.08f) * visibility;
    ctx.FillRoundedRect(track, 4.0f, D2D1::ColorF(trackBase.r, trackBase.g, trackBase.b, trackAlpha));

    float thumbAlpha = (m_isDraggingThumb ? 0.75f : (m_scrollbarHovered ? 0.55f : 0.40f)) * visibility;
    ctx.FillRoundedRect(thumb, 4.0f, D2D1::ColorF(thumbBase.r, thumbBase.g, thumbBase.b, thumbAlpha));
}

void ScrollViewer::SyncRenderState() {
    UIElement::SyncRenderState();
    UpdateContentLayerState();
    // Intentionally do NOT Validate()/clear content-layer dirty here.
    // Clearing before paint discarded theme refreshes and left stale glyphs
    // (e.g. dark-mode white text composited onto a light pane).
}

void ScrollViewer::OnThemeChanged() {
    UIElement::OnThemeChanged();
    m_contentLayer.ResetCache();
    m_contentLayerDirty.Clear();
    UpdateContentLayerState();
    MarkContentLayerDirty();
}

void ScrollViewer::MarkRenderContentDirty() {
    UIElement::MarkRenderContentDirty();
    MarkContentLayerDirty();
}

void ScrollViewer::MarkRenderRectDirty(const Rect& rect) {
    // Localized dirties (nav ripple, hover, caret) must NOT StructureDirty the
    // whole content bitmap — that re-rasters every PropertyGrid/menu control each
    // animation frame (identical hitch to the old scroll bug).
    if (!rect.IsEmpty()) {
        const Rect contentViewport = GetContentViewportRect();
        if (!contentViewport.IsEmpty() && rect.Intersects(contentViewport)) {
            m_contentLayer.Invalidate(RenderLayer::ContentDirty);
            m_contentLayerDirty.AddRect(rect);
        }
    }
    UIElement::MarkRenderRectDirty(rect);
}

void ScrollViewer::SwallowDescendantRenderDirties() {
    auto clearDeep = [](auto&& self, UIElement* el) -> void {
        if (!el) {
            return;
        }
        (void)el->GetRenderNode().ConsumeWorldDirtyRegion();
        for (auto& child : el->GetChildren()) {
            if (child) {
                self(self, child.get());
            }
        }
    };
    for (auto& child : GetChildren()) {
        if (child) {
            clearDeep(clearDeep, child.get());
        }
    }
}

void ScrollViewer::CollectRenderDirtyRegion(DirtyRegion& dirtyRegion, bool consume) {
    // Self / explicit scroll-viewport dirties.
    if (m_subtreeRenderDirty || !m_renderNode.GetWorldDirtyRegion().IsEmpty()) {
        if (consume) {
            dirtyRegion.UnionWith(m_renderNode.ConsumeWorldDirtyRegion());
            m_subtreeRenderDirty = false;
        } else {
            dirtyRegion.UnionWith(m_renderNode.GetWorldDirtyRegion());
        }
    }

    // Children may dirty after expand/collapse or control edits. Collect them into a
    // temp region — never union raw child bounds into the window (PropertyGrid doc
    // height). Any real child dirty means the content-layer bitmap must rebuild;
    // the window only needs the viewport refreshed.
    DirtyRegion childRegion;
    for (auto& child : GetChildren()) {
        if (child) {
            child->CollectRenderDirtyRegion(childRegion, consume);
        }
    }
    if (!childRegion.IsEmpty()) {
        const Rect viewport = GetContentViewportRect();
        if (consume) {
            // Soft content dirty only. Add the *individual* child dirty rects — using
            // GetBounds() of a tall PropertyGrid document made every hover look like a
            // full-viewport dirty (85% rule) and FULL_RERASTER'd every frame.
            m_contentLayer.Invalidate(RenderLayer::ContentDirty);
        }
        for (const Rect& r : childRegion.GetRects()) {
            if (r.IsEmpty()) {
                continue;
            }
            if (consume) {
                m_contentLayerDirty.AddRect(r.Inflate(2.0f));
            }
            if (!viewport.IsEmpty()) {
                const float x0 = (std::max)(r.x, viewport.x);
                const float y0 = (std::max)(r.y, viewport.y);
                const float x1 = (std::min)(r.x + r.width, viewport.x + viewport.width);
                const float y1 = (std::min)(r.y + r.height, viewport.y + viewport.height);
                if (x1 > x0 && y1 > y0) {
                    dirtyRegion.AddRect(Rect(x0, y0, x1 - x0, y1 - y0).Inflate(2.0f));
                }
            }
        }
        if (!viewport.IsEmpty() && dirtyRegion.IsEmpty()) {
            dirtyRegion.AddRect(viewport.Inflate(2.0f));
        }
    }

    if (consume) {
        // Publish content dirties to the window invalidate list, but keep
        // m_contentLayerDirty until RenderContentLayer consumes it (strip/full).
        dirtyRegion.UnionWith(m_contentLayerDirty);
    }
    // Probe (consume=false) must NOT union m_contentLayerDirty: that queue stays
    // until the next paint, and mouse-move probes would otherwise Present every
    // pixel at display refresh while waving over empty content.
}

UIElement* ScrollViewer::HitTest(float x, float y) {
    if (GetVisibility() != Visibility::Visible) return nullptr;

    // Overlays (e.g. open ComboBox) may extend past the viewport — test first.
    UIElement* overlayHit = HitTestOverlay(x, y);
    if (overlayHit) return overlayHit;

    // Match render clipping: scrolled children arranged at y-offset must NOT
    // receive hits outside the viewport (otherwise they steal TitleBar clicks).
    if (!m_bounds.Contains(x, y)) {
        return nullptr;
    }

    if (m_contentHeight > m_bounds.height && GetScrollbarTrackRect().Contains(x, y)) {
        return this;
    }

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        UIElement* hit = (*it)->HitTest(x, y);
        if (hit) return hit;
    }

    return this;
}

HCURSOR ScrollViewer::GetCursor() const {
    if (m_middleScrollActive) {
        const float dy = m_middleLastMouse.y - m_middleOrigin.y;
        return MiddleClickAutoscroll::CursorForDelta(0.0f, dy, false, GetMaxScroll() > 0.0f);
    }
    if (m_scrollbarHovered || m_isDraggingThumb) {
        return LoadCursor(nullptr, IDC_ARROW);
    }
    return nullptr;
}

void ScrollViewer::OnMouseDown(Point pt) {
    UIElement::OnMouseDown(pt);

    if (m_contentHeight <= m_bounds.height) return;

    Rect track = GetScrollbarTrackRect();
    Rect thumb = GetScrollbarThumbRect();

    if (thumb.Contains(pt.x, pt.y) || track.Contains(pt.x, pt.y)) {
        m_scrollbarAutoHide.NotifyActivity(this);
        RequestAnimationTicks();
    }

    if (thumb.Contains(pt.x, pt.y)) {
        StopSmoothScroll();
        m_isDraggingThumb = true;
        m_scrollbarAutoHide.SetDragging(true, this);
        m_dragStartY = pt.y;
        m_dragStartOffsetY = m_offsetY;
        return;
    }

    if (track.Contains(pt.x, pt.y)) {
        StopSmoothScroll();
        float maxScroll = GetMaxScroll();
        float trackH = track.height;
        float thumbH = thumb.height;
        float clickRatio = (pt.y - track.y - thumbH * 0.5f) / (std::max)(1.0f, trackH - thumbH);
        clickRatio = std::clamp(clickRatio, 0.0f, 1.0f);
        float previousOffset = m_offsetY;
        m_offsetY = clickRatio * maxScroll;
        ClampOffset();
        m_scrollAnimator.JumpTo(m_offsetY);
        PositionChildren();
        if (std::abs(previousOffset - m_offsetY) > 0.01f) {
            MarkScrollVisualDirty(previousOffset);
        }
        UIElement::MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
    }
}

void ScrollViewer::OnMouseMove(Point pt) {
    UIElement::OnMouseMove(pt);

    if (m_middleScrollActive) {
        m_middleLastMouse = pt;
        ::SetCursor(GetCursor());
        RequestAnimationTicks();
        // Keep the origin badge + cursor responsive while mouse wanders.
        const float r = MiddleClickAutoscroll::kIndicatorRadius + 4.0f;
        UIElement::MarkRenderRectDirty(Rect(m_middleOrigin.x - r, m_middleOrigin.y - r, r * 2.0f, r * 2.0f));
        return;
    }

    bool wasHovered = m_scrollbarHovered;
    m_scrollbarHovered = (m_contentHeight > m_bounds.height) && GetScrollbarTrackRect().Contains(pt.x, pt.y);
    m_scrollbarAutoHide.SetPointerOver(m_scrollbarHovered, this);
    if (wasHovered != m_scrollbarHovered) {
        UIElement::MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
        RequestAnimationTicks();
    }

    if (m_isDraggingThumb && m_isPressed) {
        float previousOffset = m_offsetY;
        float maxScroll = GetMaxScroll();
        Rect track = GetScrollbarTrackRect();
        Rect thumb = GetScrollbarThumbRect();
        float scrollableTrack = (std::max)(1.0f, track.height - thumb.height);
        float deltaY = pt.y - m_dragStartY;
        m_offsetY = m_dragStartOffsetY + (deltaY / scrollableTrack) * maxScroll;
        ClampOffset();
        m_scrollAnimator.JumpTo(m_offsetY);
        PositionChildren();
        if (std::abs(previousOffset - m_offsetY) > 0.01f) {
            MarkScrollVisualDirty(previousOffset);
        }
    }
}

void ScrollViewer::OnMouseUp(Point pt) {
    UIElement::OnMouseUp(pt);
    if (m_isDraggingThumb) {
        UIElement::MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
        RequestAnimationTicks();
    }
    m_isDraggingThumb = false;
    m_scrollbarAutoHide.SetDragging(false, this);
}

void ScrollViewer::OnMouseLeave() {
    UIElement::OnMouseLeave();
    if (m_scrollbarHovered) {
        m_scrollbarHovered = false;
        UIElement::MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
    }
    m_scrollbarAutoHide.SetPointerOver(false, this);
    RequestAnimationTicks();
}

bool ScrollViewer::OnMiddleButtonDown(Point pt) {
    if (GetMaxScroll() <= 0.0f) return false;
    if (!m_bounds.Contains(pt.x, pt.y)) return false;

    m_middleScrollActive = true;
    m_middleOrigin = pt;
    m_middleLastMouse = pt;
    StopSmoothScroll();
    m_scrollbarAutoHide.NotifyActivity(this);
    RequestAnimationTicks();
    ::SetCursor(GetCursor());
    const float r = MiddleClickAutoscroll::kIndicatorRadius + 4.0f;
    UIElement::MarkRenderRectDirty(Rect(m_middleOrigin.x - r, m_middleOrigin.y - r, r * 2.0f, r * 2.0f));
    return true;
}

void ScrollViewer::OnMiddleButtonUp(Point pt) {
    (void)pt;
    if (!m_middleScrollActive) return;
    m_middleScrollActive = false;
    const float r = MiddleClickAutoscroll::kIndicatorRadius + 4.0f;
    UIElement::MarkRenderRectDirty(Rect(m_middleOrigin.x - r, m_middleOrigin.y - r, r * 2.0f, r * 2.0f));
    RequestAnimationTicks();
}

void ScrollViewer::OnAutoScrollTick() {
    if (!m_middleScrollActive) return;

    const float dt = UIElement::GetAnimationDeltaSeconds();
    if (dt <= 0.0f) return;

    const float dy = m_middleLastMouse.y - m_middleOrigin.y;
    const float vel = MiddleClickAutoscroll::VelocityFromDelta(dy);
    if (std::abs(vel) < 0.01f) {
        RequestAnimationTicks();
        return;
    }

    float previousOffset = m_offsetY;
    StopSmoothScroll();
    m_offsetY = m_offsetY + vel * dt;
    ClampOffset();
    m_scrollAnimator.JumpTo(m_offsetY);
    PositionChildren();
    if (std::abs(previousOffset - m_offsetY) > 0.01f) {
        MarkScrollVisualDirty(previousOffset);
    }
    m_scrollbarAutoHide.NotifyActivity(this);
    RequestAnimationTicks();
}

void ScrollViewer::OnMouseWheel(float delta) {
    float maxScroll = GetMaxScroll();
    if (maxScroll <= 0.0f) {
        // No scrollable space — swallow the event instead of bubbling to parent.
        // Bubbling when embedded in NavigationView causes infinite recursion:
        // NavigationView → ScrollViewer → UIElement → parent (NavigationView) → loop.
        return;
    }

    m_scrollbarAutoHide.NotifyActivity(this);
    RequestAnimationTicks();

    if (!UIElement::AreAnimationsEnabled()) {
        float previousOffset = m_offsetY;
        m_offsetY = std::clamp(m_offsetY - delta * GetChromiumWheelStep(m_bounds.height), 0.0f, maxScroll);
        m_scrollAnimator.JumpTo(m_offsetY);
        PositionChildren();
        if (std::abs(previousOffset - m_offsetY) > 0.01f) {
            MarkScrollVisualDirty(previousOffset);
        }
        // No animation registration — still must schedule a paint or the wheel is a no-op.
        if (FrameScheduler* sched = FrameScheduler::Current()) {
            sched->ScheduleFrame();
        }
        return;
    }

    m_scrollAnimator.ScrollBy(-delta * GetChromiumWheelStep(m_bounds.height), 0.0f, maxScroll);

    if (m_lastAnimQpc == 0) {
        LARGE_INTEGER now = {};
        QueryPerformanceCounter(&now);
        m_lastAnimQpc = now.QuadPart;
    }
    RequestAnimationTicks();
}

bool ScrollViewer::AdvanceSmoothScroll() {
    if (m_isDraggingThumb) {
        StopSmoothScroll();
        return false;
    }

    double dt = SecondsSinceLastTick();
    bool advanced = m_scrollAnimator.Tick(dt, 0.0f, GetMaxScroll());
    if (!advanced) {
        m_lastAnimQpc = 0;
        return false;
    }

    float previousOffset = m_offsetY;
    m_offsetY = m_scrollAnimator.Current();
    ClampOffset();
    PositionChildren();
    if (std::abs(previousOffset - m_offsetY) > 0.01f) {
        MarkScrollVisualDirty(previousOffset);
        m_scrollbarAutoHide.NotifyActivity(this);
    }
    return true;
}

bool ScrollViewer::OnAnimationTick() {
    bool childAnimating = UIElement::OnAnimationTick();
    const float dt = UIElement::GetAnimationDeltaSeconds();

    if (!UIElement::AreAnimationsEnabled()) {
        if (m_scrollAnimator.IsActive()) {
            float previousOffset = m_offsetY;
            m_offsetY = m_scrollAnimator.Target();
            ClampOffset();
            PositionChildren();
            m_scrollAnimator.JumpTo(m_offsetY);
            if (std::abs(previousOffset - m_offsetY) > 0.01f) {
                MarkScrollVisualDirty(previousOffset);
            }
        }
        // Still drive auto-hide (instant snap) when animations are disabled.
        const float prevOpacity = m_scrollbarAutoHide.Opacity();
        const bool hideAnimating = m_scrollbarAutoHide.Tick(dt);
        if (std::abs(prevOpacity - m_scrollbarAutoHide.Opacity()) > 0.001f) {
            UIElement::MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
        }
        if (hideAnimating) {
            RequestAnimationTicks();
        }
        return childAnimating || hideAnimating;
    }

    bool selfAnimating = AdvanceSmoothScroll();
    const float prevOpacity = m_scrollbarAutoHide.Opacity();
    const bool hideAnimating = m_scrollbarAutoHide.Tick(dt);
    if (std::abs(prevOpacity - m_scrollbarAutoHide.Opacity()) > 0.001f) {
        UIElement::MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
    }
    if (selfAnimating || hideAnimating) {
        RequestAnimationTicks();
    }
    return childAnimating || selfAnimating || hideAnimating;
}

bool ScrollViewer::HasSelfAnimation() const {
    return (UIElement::AreAnimationsEnabled() && m_scrollAnimator.IsActive())
        || m_scrollbarAutoHide.NeedsTicks()
        || m_middleScrollActive;
}

void ScrollViewer::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (m_visibility != Visibility::Visible) {
        return;
    }
    // Never union the full ScrollViewer/PropertyGrid bounds — that forced 整帧
    // paints for every scrollbar fade tick and made the whole app feel stuck.
    if (UIElement::AreAnimationsEnabled() && m_scrollAnimator.IsActive()) {
        const Rect vp = GetContentViewportRect();
        if (!vp.IsEmpty()) {
            dirtyRect = hasDirty ? dirtyRect.Union(vp) : vp;
            hasDirty = true;
        }
        return;
    }
    // Only while opacity is changing — IsDrawn() used to dirty the full-height
    // track on every registered frame (e.g. tooltip wait), which felt like
    // "scrollbar visible = busy pump".
    if (m_scrollbarAutoHide.NeedsTicks()) {
        const Rect track = GetScrollbarTrackRect().Inflate(2.0f);
        if (!track.IsEmpty()) {
            dirtyRect = hasDirty ? dirtyRect.Union(track) : track;
            hasDirty = true;
        }
    }
}

} // namespace CUI
