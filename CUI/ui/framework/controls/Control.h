#pragma once
#include "UIElement.h"
#include "../animation/AnimationSystem.h"

namespace CUI {

class Control : public UIElement {
public:
    Control();
    virtual ~Control() = default;

    virtual const char* GetClassName() const override { return "Control"; }

    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    virtual void OnMouseEnter() override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;

protected:
    D2D1_COLOR_F GetAnimatedBackground(D2D1_COLOR_F fallback);
    D2D1_COLOR_F BlendColor(D2D1_COLOR_F from, D2D1_COLOR_F to, float t) const;
    void UpdateVisualStateTarget();
    float GetVisualStateTarget() const;

    AnimatedScalar m_visualStateAnim{};
    float m_visualStateTarget = 0.0f;
};

} // namespace CUI
