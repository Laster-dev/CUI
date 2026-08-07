#pragma once
#include "Control.h"
#include "../render/RenderLayer.h"
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
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;

    float GetValue() const { return m_value; }
    void SetValue(float val) {
        m_value = val;
        NotifyFieldChanged(PropertyId::ControlValue, Value(val));
        RequestAnimationTicks();
    }

    float GetMinimum() const { return m_minimum; }
    void SetMinimum(float minVal) {
        m_minimum = minVal;
        NotifyFieldChanged(PropertyId::Minimum, Value(minVal));
    }

    float GetMaximum() const { return m_maximum; }
    void SetMaximum(float maxVal) {
        m_maximum = maxVal;
        NotifyFieldChanged(PropertyId::Maximum, Value(maxVal));
    }

    bool IsIndeterminate() const { return m_isIndeterminate; }
    void SetIsIndeterminate(bool ind) {
        m_isIndeterminate = ind;
        NotifyFieldChanged(PropertyId::IsIndeterminate, Value(ind));
        m_indicatorCacheValid = false;
        if (ind) {
            RequestAnimationTicks();
        }
    }

private:
    void InvalidateIndicatorCache();
    bool EnsureIndicatorStrip(GraphicsContext& ctx, float trackW, float trackH, float radius, D2D1_COLOR_F fill);

    float m_value = 0.0f;
    float m_minimum = 0.0f;
    float m_maximum = 100.0f;
    bool m_isIndeterminate = false;
    float m_animOffset = 0.0f;
    float m_displayValue = 0.0f;
    float m_slideX = 0.0f;
    float m_prevSlideX = 0.0f;
    RenderLayer m_indicatorLayer;
    bool m_indicatorCacheValid = false;
    float m_cachedStripW = 0.0f;
    float m_cachedStripH = 0.0f;
    D2D1_COLOR_F m_cachedFillColor{};
    std::chrono::steady_clock::time_point m_lastTickTime{};
};

} // namespace CUI
