#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ScrollViewer.h"
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
}

ScrollViewer::ScrollViewer() {
    SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));
    QueryPerformanceFrequency(&m_qpcFreq);
    m_scrollAnimator.Reset(0.0f);
}

void ScrollViewer::SetScrollOffsetY(float offset) {
    StopSmoothScroll();
    m_offsetY = offset;
    ClampOffset();
    m_scrollAnimator.JumpTo(m_offsetY);
    PositionChildren();
}

void ScrollViewer::StopSmoothScroll() {
    m_scrollAnimator.JumpTo(m_offsetY);
    m_lastAnimQpc = 0;
}

float ScrollViewer::GetMaxScroll() const {
    return (std::max)(0.0f, m_contentHeight - m_bounds.height);
}

float ScrollViewer::GetScrollbarReserve() const {
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
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));
    float height = 0.0f;
    Size avail(std::max(0.0f, contentWidth), 100000.0f);
    for (auto& child : GetChildren()) {
        std::string vis = child->GetProperty("visibility").AsString("Visible");
        if (vis == "Collapsed") continue;
        Size dSize = child->Measure(avail);
        height = std::max(height, dSize.height);
    }
    m_measuredContentWidth = contentWidth;
    return padding.top + height + padding.bottom + kContentBottomPad;
}

void ScrollViewer::RefreshContentMetrics(float viewportWidth, float viewportHeight) {
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));
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
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));
    float innerWidth = std::max(0.0f, m_bounds.width - padding.left - padding.right);
    float reserve = GetScrollbarReserve();
    float childWidth = std::max(0.0f, innerWidth - reserve);
    float viewportContentHeight = std::max(0.0f, m_bounds.height - padding.top - padding.bottom);

    // Scrolling changes only child positions. Do not Measure here: thumb dragging
    // and inertial animation run at frame rate.
    float naturalHeight = 0.0f;
    for (auto& child : GetChildren()) {
        std::string vis = child->GetProperty("visibility").AsString("Visible");
        if (vis == "Collapsed") continue;
        naturalHeight = std::max(naturalHeight, child->GetDesiredSize().height);
    }
    float childHeight = std::max(viewportContentHeight, naturalHeight);

    for (auto& child : GetChildren()) {
        std::string vis = child->GetProperty("visibility").AsString("Visible");
        if (vis == "Collapsed") continue;
        Rect childRect(
            m_bounds.x + padding.left,
            m_bounds.y + padding.top - m_offsetY,
            childWidth,
            childHeight
        );
        child->Arrange(childRect);
    }
}

Size ScrollViewer::Measure(Size availableSize) {
    Thickness margin = GetProperty("margin").AsThickness(Thickness(0));
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));

    float expW = GetProperty("width").AsFloat(-1.0f);
    float expH = GetProperty("height").AsFloat(-1.0f);

    // Use the control's own width when explicit — NOT the parent's full available width.
    // Measuring with the window width was underestimating wrapped content for a 320px panel.
    float finalW = (expW >= 0.0f) ? expW : availableSize.width;
    float finalH = (expH >= 0.0f) ? expH : availableSize.height;
    RefreshContentMetrics(finalW - margin.left - margin.right, finalH - margin.top - margin.bottom);

    m_desiredSize = Size(finalW, finalH);
    return m_desiredSize;
}

void ScrollViewer::Arrange(Rect finalRect) {
    m_bounds = finalRect;
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));
    float reserve = GetScrollbarReserve();
    float contentWidth = std::max(
        0.0f,
        finalRect.width - padding.left - padding.right - reserve
    );
    if (std::abs(contentWidth - m_measuredContentWidth) > 0.5f) {
        RefreshContentMetrics(finalRect.width, finalRect.height);
    }
    PositionChildren();
}

void ScrollViewer::Render(GraphicsContext& ctx) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return;

    ctx.PushClip(m_bounds);
    OnRender(ctx);

    for (auto& child : GetChildren()) {
        child->Render(ctx);
    }

    if (m_contentHeight > m_bounds.height && m_bounds.height > 0.0f) {
        Rect track = GetScrollbarTrackRect();
        Rect thumb = GetScrollbarThumbRect();

        float trackAlpha = m_scrollbarHovered || m_isDraggingThumb ? 0.18f : 0.08f;
        ctx.FillRoundedRect(track, 4.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, trackAlpha));

        float thumbAlpha = m_isDraggingThumb ? 0.75f : (m_scrollbarHovered ? 0.55f : 0.40f);
        ctx.FillRoundedRect(thumb, 4.0f, D2D1::ColorF(0x79 / 255.0f, 0x79 / 255.0f, 0x79 / 255.0f, thumbAlpha));
    }

    ctx.PopClip();
}

void ScrollViewer::OnRender(GraphicsContext& ctx) {
    UIElement::OnRender(ctx);
}

UIElement* ScrollViewer::HitTest(float x, float y) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return nullptr;

    if (m_contentHeight > m_bounds.height && GetScrollbarTrackRect().Contains(x, y)) {
        return this;
    }

    UIElement* overlayHit = HitTestOverlay(x, y);
    if (overlayHit) return overlayHit;

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        UIElement* hit = (*it)->HitTest(x, y);
        if (hit) return hit;
    }

    if (m_bounds.Contains(x, y)) {
        return this;
    }
    return nullptr;
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

    if (thumb.Contains(pt.x, pt.y)) {
        StopSmoothScroll();
        m_isDraggingThumb = true;
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
        m_offsetY = clickRatio * maxScroll;
        ClampOffset();
        m_scrollAnimator.JumpTo(m_offsetY);
        PositionChildren();
    }
}

void ScrollViewer::OnMouseMove(Point pt) {
    UIElement::OnMouseMove(pt);

    m_scrollbarHovered = (m_contentHeight > m_bounds.height) && GetScrollbarTrackRect().Contains(pt.x, pt.y);

    if (m_isDraggingThumb && m_isPressed) {
        float maxScroll = GetMaxScroll();
        Rect track = GetScrollbarTrackRect();
        Rect thumb = GetScrollbarThumbRect();
        float scrollableTrack = (std::max)(1.0f, track.height - thumb.height);
        float deltaY = pt.y - m_dragStartY;
        m_offsetY = m_dragStartOffsetY + (deltaY / scrollableTrack) * maxScroll;
        ClampOffset();
        m_scrollAnimator.JumpTo(m_offsetY);
        PositionChildren();
    }
}

void ScrollViewer::OnMouseUp(Point pt) {
    UIElement::OnMouseUp(pt);
    m_isDraggingThumb = false;
}

void ScrollViewer::OnMouseWheel(float delta) {
    float maxScroll = GetMaxScroll();
    if (maxScroll <= 0.0f) {
        UIElement::OnMouseWheel(delta);
        return;
    }

    m_scrollAnimator.ScrollBy(-delta * GetChromiumWheelStep(m_bounds.height), 0.0f, maxScroll);

    if (m_lastAnimQpc == 0) {
        LARGE_INTEGER now = {};
        QueryPerformanceCounter(&now);
        m_lastAnimQpc = now.QuadPart;
    }
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

    m_offsetY = m_scrollAnimator.Current();
    ClampOffset();
    PositionChildren();
    return true;
}

bool ScrollViewer::OnAnimationTick() {
    bool childAnimating = UIElement::OnAnimationTick();
    bool selfAnimating = AdvanceSmoothScroll();
    return childAnimating || selfAnimating;
}

bool ScrollViewer::HasSelfAnimation() const {
    return m_scrollAnimator.IsActive();
}

} // namespace CUI
