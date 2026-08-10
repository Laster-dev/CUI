#pragma once
#include "Control.h"
#include "TextBox.h"
#include "Button.h"

namespace CUI {

class NumberBox : public Control {
public:
    NumberBox();
    virtual ~NumberBox() = default;

    virtual const char* GetClassName() const override { return "NumberBox"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnKeyDown(int vkCode) override;

    float GetValue() const { return m_value; }
    void SetValue(float val);

    float GetStep() const { return m_step; }
    void SetStep(float s) {
        m_step = s;
        NotifyFieldChanged(PropertyId::Step, Value(s));
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

    Event<NumberBox*, float>& OnValueChanged() { return m_onValueChangedEvent; }

private:
    float m_value = 0.0f;
    float m_minimum = -100000.0f;
    float m_maximum = 100000.0f;
    float m_step = 1.0f;
    std::shared_ptr<TextBox> m_inputBox;
    std::shared_ptr<Button> m_btnUp;
    std::shared_ptr<Button> m_btnDown;
    Event<NumberBox*, float> m_onValueChangedEvent;
};

} // namespace CUI
