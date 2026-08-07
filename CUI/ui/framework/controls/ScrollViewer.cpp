#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ScrollViewer.h"
#include "../render/CompositionContext.h"
#include "../style/ThemeManager.h"
#include <wrl/client.h>
#include <algorithm>
#include <cmath>

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
    OnPropertyIdChanged().Connect([this](PropertyId, const Value&) {
        MarkContentLayerDirty();
    });
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
    m_measuredContentWidth = contentWidth;
    MarkContentLayerDirty();
    return padding.top + height + padding.bottom + kContentBottomPad;
}

void ScrollViewer::RefreshContentMetrics(float viewportWidth, float viewportHeight) {
    Thickness padding = GetPadding();
    float innerWidth = std::max(0.0f, viewportWidth - padding.left - padding.right);

    // First measure without a scrollbar. If it overflows, measure once more with
    // the reserved scrollbar gutter because narrower text may wrap higher.
    m_contentHeight = MeasureContentHeight(innerWidth);
    if (m_contentHeight > viewportHeight) {
        float contentWidth = std::max(0.0f, innerWidth - kScrollbarInset - kScrollbarWidth);
        m_contentHeight = MeasureContentHeight(contentWidth);
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
    float childHeight = std::max(viewportContentHeight, naturalHeight);

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
        if (visualContentHeight > m_contentHeight + 0.5f) {
            // Some layout containers intentionally render children past their own
            // bounds (PropertyGrid does this to avoid clipping rows). Scroll range
            // must follow the real visual subtree, not only the direct child's
            // desired height, or the last controls cannot be scrolled into view.
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
    m_contentLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty);
    m_pendingViewportScrollPatch = false;
    m_pendingViewportPatchDeltaY = 0.0f;
    Rect viewport = GetContentViewportRect();
    if (!viewport.IsEmpty()) {
        m_contentLayerDirty.AddRect(viewport.Inflate(2.0f));
        MarkRenderRectDirty(viewport.Inflate(2.0f));
    }
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
    if (m_contentLayer.HasDirtyFlags()) {
        return true;
    }
    if (!ctx.IntersectsPaintBounds(m_contentViewportRect)) {
        return true;
    }
    return m_contentLayerDirty.GetRectCount() != 1;
}

void ScrollViewer::MarkScrollVisualDirty(float previousOffset) {
    if (m_bounds.IsEmpty()) {
        return;
    }

    Rect contentViewport = GetContentViewportRect();
    if (!contentViewport.IsEmpty()) {
        MarkContentLayerRectDirty(contentViewport.Inflate(2.0f));
        m_contentLayer.Invalidate(RenderLayer::TransformDirty);
        m_pendingViewportScrollPatch = true;
        m_pendingViewportPatchDeltaY = m_offsetY - previousOffset;
    }

    if (m_contentHeight > m_bounds.height) {
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));

        float maxScroll = GetMaxScroll();
        if (maxScroll > 0.0f) {
            Rect track = GetScrollbarTrackRect();
            float thumbHeight = (m_bounds.height / m_contentHeight) * track.height;
            thumbHeight = std::clamp(thumbHeight, 24.0f, track.height);
            float previousRatio = std::clamp(previousOffset / maxScroll, 0.0f, 1.0f);
            float previousThumbY = track.y + previousRatio * (track.height - thumbHeight);
            MarkRenderRectDirty(Rect(track.x, previousThumbY, track.width, thumbHeight).Inflate(2.0f));
            MarkRenderRectDirty(GetScrollbarThumbRect().Inflate(2.0f));
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
    RenderContentLayer(ctx);
    RenderScrollChrome(ctx);

    ctx.PopClip();
}

void ScrollViewer::OnRender(GraphicsContext& ctx) {
    UIElement::OnRender(ctx);
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

    const bool modeChanged = m_contentLayerCachesFullContent != cacheFullContent;
    const bool sizeChanged =
        std::abs(m_contentLayer.GetCacheSurfaceSize().width - cacheSize.width) > 0.5f
        || std::abs(m_contentLayer.GetCacheSurfaceSize().height - cacheSize.height) > 0.5f;
    const bool canPatchViewportCache =
        !cacheFullContent
        && !modeChanged
        && !sizeChanged
        && m_contentLayer.IsValid()
        && m_contentLayer.GetCacheBitmap() != nullptr
        && m_pendingViewportScrollPatch
        && std::abs(m_pendingViewportPatchDeltaY) > 0.01f
        && std::abs(m_pendingViewportPatchDeltaY) < m_contentViewportRect.height;
    const bool needsRerender = modeChanged || sizeChanged || ShouldRenderFullContentLayer(ctx);
    if (auto* composition = ctx.GetCompositionContext()) {
        if (canPatchViewportCache) {
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

void ScrollViewer::MarkRenderContentDirty() {
    UIElement::MarkRenderContentDirty();
    MarkContentLayerDirty();
}

void ScrollViewer::MarkRenderRectDirty(const Rect& rect) {
    // Nav-item ripples / hover mark local rects; without this the content-layer
    // cache stays stale and the ripple looks frozen until something else rebuilds it.
    if (!rect.IsEmpty()) {
        m_contentLayer.Invalidate(RenderLayer::ContentDirty);
        m_contentLayerDirty.AddRect(rect);
    }
    UIElement::MarkRenderRectDirty(rect);
}

void ScrollViewer::CollectRenderDirtyRegion(DirtyRegion& dirtyRegion, bool consume) {
    UIElement::CollectRenderDirtyRegion(dirtyRegion, consume);
    if (consume) {
        dirtyRegion.UnionWith(m_contentLayerDirty);
        m_contentLayerDirty.Clear();
    } else {
        dirtyRegion.UnionWith(m_contentLayerDirty);
    }
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
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
    }
}

void ScrollViewer::OnMouseMove(Point pt) {
    UIElement::OnMouseMove(pt);

    bool wasHovered = m_scrollbarHovered;
    m_scrollbarHovered = (m_contentHeight > m_bounds.height) && GetScrollbarTrackRect().Contains(pt.x, pt.y);
    m_scrollbarAutoHide.SetPointerOver(m_scrollbarHovered, this);
    if (wasHovered != m_scrollbarHovered) {
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
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
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
        RequestAnimationTicks();
    }
    m_isDraggingThumb = false;
    m_scrollbarAutoHide.SetDragging(false, this);
}

void ScrollViewer::OnMouseLeave() {
    UIElement::OnMouseLeave();
    if (m_scrollbarHovered) {
        m_scrollbarHovered = false;
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
    }
    m_scrollbarAutoHide.SetPointerOver(false, this);
    RequestAnimationTicks();
}

void ScrollViewer::OnMouseWheel(float delta) {
    float maxScroll = GetMaxScroll();
    if (maxScroll <= 0.0f) {
        UIElement::OnMouseWheel(delta);
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
            MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
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
        MarkRenderRectDirty(GetScrollbarTrackRect().Inflate(2.0f));
    }
    if (selfAnimating || hideAnimating) {
        RequestAnimationTicks();
    }
    return childAnimating || selfAnimating || hideAnimating;
}

bool ScrollViewer::HasSelfAnimation() const {
    return (UIElement::AreAnimationsEnabled() && m_scrollAnimator.IsActive())
        || m_scrollbarAutoHide.NeedsTicks();
}

} // namespace CUI
