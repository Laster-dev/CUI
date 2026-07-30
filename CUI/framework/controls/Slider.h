#pragma once
#include "Control.h"

namespace CUI {

class Slider : public Control {
public:
    Slider();
    virtual ~Slider() = default;

    virtual const char* GetClassName() const override { return "Slider"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;

    float GetValue() const { return GetProperty("value").AsFloat(0.0f); }
    void SetValue(float val);

    float GetMinimum() const { return GetProperty("minimum").AsFloat(0.0f); }
    void SetMinimum(float minVal) { SetProperty("minimum", Value(minVal)); }

    float GetMaximum() const { return GetProperty("maximum").AsFloat(100.0f); }
    void SetMaximum(float maxVal) { SetProperty("maximum", Value(maxVal)); }

    float GetStep() const { return GetProperty("step").AsFloat(1.0f); }
    void SetStep(float s) { SetProperty("step", Value(s)); }

    Event<Slider*, float>& OnValueChanged() { return m_onValueChangedEvent; }

private:
    void UpdateValueFromPoint(Point pt);
    Rect GetThumbRect() const;
    Rect GetTrackRect() const;

    bool m_isDragging = false;
    float m_displayValue = 0.0f;
    Event<Slider*, float> m_onValueChangedEvent;
};

} // namespace CUI
