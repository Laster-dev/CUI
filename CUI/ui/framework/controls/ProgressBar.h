#pragma once
#include "Control.h"
#include <chrono>

namespace CUI {

class ProgressBar : public Control {
public:
    ProgressBar();
    virtual ~ProgressBar() = default;

    virtual const char* GetClassName() const override { return "ProgressBar"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    float GetValue() const { return m_value; }
    void SetValue(float val) {
        m_value = val;
        NotifyFieldChanged(PropertyId::ControlValue, Value(val));
        // Before first layout (or with anims off) snap — otherwise display stays 0
        // until a click starts the pump ("only shows after click").
        if (!UIElement::AreAnimationsEnabled() || m_bounds.IsEmpty()) {
            m_displayValue = val;
        }
        MarkRenderRectDirty(m_bounds);
        RequestAnimationTicks();
    }

    float GetMinimum() const { return m_minimum; }
    void SetMinimum(float minVal) {
        m_minimum = minVal;
        NotifyFieldChanged(PropertyId::Minimum, Value(minVal));
        MarkRenderRectDirty(m_bounds);
    }

    float GetMaximum() const { return m_maximum; }
    void SetMaximum(float maxVal) {
        m_maximum = maxVal;
        NotifyFieldChanged(PropertyId::Maximum, Value(maxVal));
        MarkRenderRectDirty(m_bounds);
    }

    bool IsIndeterminate() const { return m_isIndeterminate; }
    void SetIsIndeterminate(bool ind) {
        m_isIndeterminate = ind;
        NotifyFieldChanged(PropertyId::IsIndeterminate, Value(ind));
        MarkRenderRectDirty(m_bounds);
        if (ind) {
            RequestAnimationTicks();
        }
    }

private:
    float m_value = 0.0f;
    float m_minimum = 0.0f;
    float m_maximum = 100.0f;
    bool m_isIndeterminate = false;
    float m_animOffset = 0.0f;
    float m_displayValue = 0.0f;
    std::chrono::steady_clock::time_point m_lastTickTime{};
};

} // namespace CUI
