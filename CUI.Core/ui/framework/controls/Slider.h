#pragma once
#include "Control.h"

namespace CUI {

class Slider : public Control {
public:
    Slider();
    virtual ~Slider() = default;

    virtual const char* GetClassName() const override { return "Slider"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    float GetValue() const { return m_value; }
    void SetValue(float val);

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

    float GetStep() const { return m_step; }
    void SetStep(float s) {
        m_step = s;
        NotifyFieldChanged(PropertyId::Step, Value(s));
    }

    Event<Slider*, float>& OnValueChanged() { return m_onValueChangedEvent; }

private:
    void UpdateValueFromPoint(Point pt);
    Rect GetThumbRect() const;
    Rect GetTrackRect() const;
    void MarkSliderVisualDirty(const Rect& previousThumb, float previousDisplayValue);

    float m_value = 0.0f;
    float m_minimum = 0.0f;
    float m_maximum = 100.0f;
    float m_step = 1.0f;
    bool m_isDragging = false;
    AnimatedScalar m_displayValueAnim{};
    Event<Slider*, float> m_onValueChangedEvent;
};

} // namespace CUI
