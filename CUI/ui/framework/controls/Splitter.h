#pragma once
#include "Control.h"

namespace CUI {

class Splitter : public Control {
public:
    Splitter();
    virtual ~Splitter() = default;

    virtual const char* GetClassName() const override { return "Splitter"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;

    std::string GetOrientation() const { return GetProperty("orientation").AsString("Vertical"); }
    void SetOrientation(const std::string& orient) { SetProperty("orientation", Value(orient)); }

    Event<Splitter*, float>& OnSplitterMoved() { return m_onSplitterMovedEvent; }

private:
    bool m_isDragging = false;
    Point m_dragStartPt;
    Event<Splitter*, float> m_onSplitterMovedEvent;
};

} // namespace CUI
