#pragma once
#include "UIElement.h"

namespace CUI {

class Control : public UIElement {
public:
    Control();
    virtual ~Control() = default;

    virtual const char* GetClassName() const override { return "Control"; }

    virtual void OnRender(GraphicsContext& ctx) override;

    virtual void OnMouseEnter() override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
};

} // namespace CUI
