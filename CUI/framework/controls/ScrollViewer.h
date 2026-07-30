#pragma once
#include "UIElement.h"
#include <cmath>

namespace CUI {

class ScrollViewer : public UIElement {
public:
    ScrollViewer();
    virtual ~ScrollViewer() = default;

    virtual const char* GetClassName() const override { return "ScrollViewer"; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual UIElement* HitTest(float x, float y) override;

    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnAnimationTick() override;
    virtual HCURSOR GetCursor() const override;

    float GetScrollOffsetY() const { return m_offsetY; }
    void SetScrollOffsetY(float offset);
    bool IsScrollAnimating() const { return std::abs(m_velocityY) > kStopSpeed; }

private:
    float GetMaxScroll() const;
    float GetScrollbarReserve() const;
    Rect GetScrollbarTrackRect() const;
    Rect GetScrollbarThumbRect() const;
    void ClampOffset();
    void StopInertia();
    bool AdvanceInertia();
    double SecondsSinceLastTick();
    float MeasureContentHeight(float contentWidth);
    void RefreshContentMetrics(float viewportWidth, float viewportHeight);
    void PositionChildren();

    float m_offsetY = 0.0f;
    float m_velocityY = 0.0f; // px / second
    float m_contentHeight = 0.0f;
    float m_measuredContentWidth = -1.0f;

    bool m_isDraggingThumb = false;
    float m_dragStartY = 0.0f;
    float m_dragStartOffsetY = 0.0f;
    bool m_scrollbarHovered = false;

    LARGE_INTEGER m_qpcFreq = {};
    LONGLONG m_lastAnimQpc = 0;

    // Leave enough inset so thumb is outside the window's ~8px resize border.
    static constexpr float kScrollbarInset = 12.0f;
    static constexpr float kScrollbarWidth = 8.0f;
    // Extra scrollable space so the last row isn't glued to the bottom edge.
    static constexpr float kContentBottomPad = 20.0f;

    static constexpr float kWheelImpulse = 1400.0f;
    static constexpr float kFriction = 9.5f;
    static constexpr float kMaxSpeed = 9000.0f;
    static constexpr float kStopSpeed = 12.0f;
};

} // namespace CUI
