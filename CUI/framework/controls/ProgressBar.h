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

    float GetValue() const { return GetProperty("value").AsFloat(0.0f); }
    void SetValue(float val) { SetProperty("value", Value(val)); }

    float GetMinimum() const { return GetProperty("minimum").AsFloat(0.0f); }
    void SetMinimum(float minVal) { SetProperty("minimum", Value(minVal)); }

    float GetMaximum() const { return GetProperty("maximum").AsFloat(100.0f); }
    void SetMaximum(float maxVal) { SetProperty("maximum", Value(maxVal)); }

    bool IsIndeterminate() const { return GetProperty("isIndeterminate").AsBool(false); }
    void SetIsIndeterminate(bool ind) { SetProperty("isIndeterminate", Value(ind)); }

private:
    float m_animOffset = 0.0f;
    float m_displayValue = 0.0f;
    std::chrono::steady_clock::time_point m_lastTickTime{};
};

} // namespace CUI
