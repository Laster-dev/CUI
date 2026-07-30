#pragma once
#include "UIElement.h"

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
    virtual void OnMouseWheel(float delta) override;

    float GetScrollOffsetY() const { return m_offsetY; }
    void SetScrollOffsetY(float offset) { m_offsetY = offset; }

private:
    float m_offsetY = 0.0f;
    float m_contentHeight = 0.0f;
};

} // namespace CUI
