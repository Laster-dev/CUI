#pragma once
#include "Control.h"
#include <chrono>
#include <memory>

namespace CUI {

struct ProgressBarCompose;

class ProgressBar : public Control {
public:
    ProgressBar();
    virtual ~ProgressBar();

    virtual const char* GetClassName() const override { return "ProgressBar"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void ApplyProperty(PropertyId id, const Value& val) override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual bool IsComposeOnlyAnimation() const override;
    virtual bool ComposePresent(GraphicsContext& ctx) override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual void OnNavigatedFrom() override;

    float GetValue() const { return m_value; }
    void ApplyValue(float val) {
        if (m_value == val) {
            return;
        }
        m_value = val;
        NotifyFieldChanged(PropertyId::ControlValue, Value(val));
        if (!UIElement::AreAnimationsEnabled() || m_bounds.IsEmpty()) {
            m_displayValue = val;
        }
        MarkRenderRectDirty(m_bounds);
        RequestAnimationTicks();
    }

    float GetMinimum() const { return m_minimum; }
    void ApplyMinimum(float minVal) {
        if (m_minimum == minVal) {
            return;
        }
        m_minimum = minVal;
        NotifyFieldChanged(PropertyId::Minimum, Value(minVal));
        MarkRenderRectDirty(m_bounds);
    }

    float GetMaximum() const { return m_maximum; }
    void ApplyMaximum(float maxVal) {
        if (m_maximum == maxVal) {
            return;
        }
        m_maximum = maxVal;
        NotifyFieldChanged(PropertyId::Maximum, Value(maxVal));
        MarkRenderRectDirty(m_bounds);
    }

    bool IsIndeterminate() const { return m_isIndeterminate; }
    void ApplyIsIndeterminate(bool ind);

private:
    void ReleaseComposeOverlay(GraphicsContext* ctx);
    bool EnsureComposeOverlay(GraphicsContext& ctx, float dpi);
    void DrawIndeterminateFallback(GraphicsContext& ctx) const;

    float m_value = 0.0f;
    float m_minimum = 0.0f;
    float m_maximum = 100.0f;
    bool m_isIndeterminate = false;
    float m_animOffset = 0.0f;
    float m_displayValue = 0.0f;
    std::chrono::steady_clock::time_point m_lastTickTime{};

    std::unique_ptr<ProgressBarCompose> m_compose;
};

} // namespace CUI
