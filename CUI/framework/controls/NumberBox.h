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

    float GetValue() const { return GetProperty("value").AsFloat(0.0f); }
    void SetValue(float val);

    float GetStep() const { return GetProperty("step").AsFloat(1.0f); }
    void SetStep(float s) { SetProperty("step", Value(s)); }

    float GetMinimum() const { return GetProperty("minimum").AsFloat(-100000.0f); }
    void SetMinimum(float minVal) { SetProperty("minimum", Value(minVal)); }

    float GetMaximum() const { return GetProperty("maximum").AsFloat(100000.0f); }
    void SetMaximum(float maxVal) { SetProperty("maximum", Value(maxVal)); }

    Event<NumberBox*, float>& OnValueChanged() { return m_onValueChangedEvent; }

private:
    std::shared_ptr<TextBox> m_inputBox;
    std::shared_ptr<Button> m_btnUp;
    std::shared_ptr<Button> m_btnDown;
    Event<NumberBox*, float> m_onValueChangedEvent;
};

} // namespace CUI
