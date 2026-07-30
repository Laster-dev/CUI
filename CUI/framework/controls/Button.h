#pragma once
#include "Control.h"

namespace CUI {

class Button : public Control {
public:
    Button();
    explicit Button(const std::string& text);
    virtual ~Button() = default;

    virtual const char* GetClassName() const override { return "Button"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

private:
    Point m_rippleCenter{};
    float m_rippleRadius = 0.0f;
    float m_rippleOpacity = 0.0f;
    bool m_rippleActive = false;
};

} // namespace CUI
