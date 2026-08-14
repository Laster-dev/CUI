#pragma once
#include "Control.h"

namespace CUI {

class Splitter : public Control {
public:
    Splitter();
    virtual ~Splitter() = default;

    virtual const char* GetClassName() const override { return "Splitter"; }
    virtual HCURSOR GetCursor() const override;

    virtual Size Measure(Size availableSize) override;
    virtual UIElement* HitTest(float x, float y) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;

    bool IsVerticalSplitter() const { return GetOrientation() == Orientation::Vertical; }
    void SetOrientation(Orientation orientation);

    Event<Splitter*, float>& OnSplitterMoved() { return m_onSplitterMovedEvent; }

private:
    bool m_isDragging = false;
    Point m_dragStartPt;
    Event<Splitter*, float> m_onSplitterMovedEvent;
};

} // namespace CUI
