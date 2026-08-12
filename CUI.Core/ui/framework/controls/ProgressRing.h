#pragma once
#include "Control.h"

namespace CUI {

class ProgressRing : public Control {
public:
    ProgressRing();
    virtual ~ProgressRing() = default;

    virtual const char* GetClassName() const override { return "ProgressRing"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnNavigatedTo() override;

    float GetValue() const { return m_value; }
    void SetValue(float val);

    float GetMinimum() const { return m_minimum; }
    void SetMinimum(float minVal);

    float GetMaximum() const { return m_maximum; }
    void SetMaximum(float maxVal);

    bool IsIndeterminate() const { return m_isIndeterminate; }
    void SetIsIndeterminate(bool ind);

private:
    void DrawRingArc(
        GraphicsContext& ctx,
        Point center,
        float radius,
        float strokeWidth,
        D2D1_COLOR_F color,
        float startRad,
        float sweepRad) const;

    float m_value = 0.0f;
    float m_minimum = 0.0f;
    float m_maximum = 100.0f;
    bool m_isIndeterminate = true;
    float m_spinAngle = 0.0f;
    float m_displayValue = 0.0f;
};

} // namespace CUI
