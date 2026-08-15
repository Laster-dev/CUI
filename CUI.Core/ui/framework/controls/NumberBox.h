#pragma once
#include "Control.h"
#include "TextBox.h"
#include <cstdint>
#include <memory>

namespace CUI {

class NumberBox : public Control {
public:
    NumberBox();
    virtual ~NumberBox() = default;

    virtual const char* GetClassName() const override { return "NumberBox"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override;
    bool AcceptsTabFocus() const override { return false; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    /**
     * @brief 数字框当前数值属性代理。
     */
    struct NumberBoxValueProperty {
        NumberBox* owner;
        NumberBoxValueProperty& operator=(float v) { owner->SetValue(v); return *this; }
        operator float() const { return owner->GetValue(); }
        float Get() const { return owner->GetValue(); }
    } Value{this};

    /**
     * @brief 数字框允许输入的下限最小值属性代理。
     */
    struct NumberBoxMinimumProperty {
        NumberBox* owner;
        NumberBoxMinimumProperty& operator=(float v) { owner->SetMinimum(v); return *this; }
        operator float() const { return owner->GetMinimum(); }
        float Get() const { return owner->GetMinimum(); }
    } Minimum{this};

    /**
     * @brief 数字框允许输入的上限最大值属性代理。
     */
    struct NumberBoxMaximumProperty {
        NumberBox* owner;
        NumberBoxMaximumProperty& operator=(float v) { owner->SetMaximum(v); return *this; }
        operator float() const { return owner->GetMaximum(); }
        float Get() const { return owner->GetMaximum(); }
    } Maximum{this};

    /**
     * @brief 微调步长增量属性代理。
     */
    struct NumberBoxStepProperty {
        NumberBox* owner;
        NumberBoxStepProperty& operator=(float v) { owner->SetStep(v); return *this; }
        operator float() const { return owner->GetStep(); }
        float Get() const { return owner->GetStep(); }
    } Step{this};

    float GetValue() const { return m_value; }
    void SetValue(float val);

    PropertyRef<float, PropertyId::ControlValue> ValueProperty; // 当前数值的响应式双向绑定属性代理

    float GetStep() const { return m_step; }
    void SetStep(float s) {
        m_step = s;
        NotifyFieldChanged(PropertyId::Step, CUI::Value(s));
    }

    float GetMinimum() const { return m_minimum; }
    void SetMinimum(float minVal) {
        m_minimum = minVal;
        NotifyFieldChanged(PropertyId::Minimum, CUI::Value(minVal));
    }

    float GetMaximum() const { return m_maximum; }
    void SetMaximum(float maxVal) {
        m_maximum = maxVal;
        NotifyFieldChanged(PropertyId::Maximum, CUI::Value(maxVal));
    }

    Event<NumberBox*, float>& OnValueChanged() { return m_onValueChangedEvent; }

    bool HandleFieldKey(int vkCode);
    void OnFieldTextChanged();
    void StepBy(float dir);
    void CommitEdit();

private:
    enum class HitPart : uint8_t { None, Text, Up, Down };

    class Field : public TextBox {
    public:
        NumberBox* host = nullptr;
        void OnRoutedEvent(RoutedEventArgs& args) override;
        void OnCharInput(wchar_t ch) override;
        void OnMouseWheel(float delta) override;
        void OnBlur() override;
    };

    static constexpr float kSpinnerW = 18.0f;

    Rect SpinnerCol() const;
    Rect UpBtn() const;
    Rect DownBtn() const;
    Rect TextRect() const;
    HitPart HitTestPart(Point pt) const;
    std::string FormatValue(float val) const;
    void SyncTextFromValue();
    void LayoutField();

    std::shared_ptr<Field> m_field;
    float m_value = 0.0f;
    float m_minimum = -100000.0f;
    float m_maximum = 100000.0f;
    float m_step = 1.0f;
    bool m_syncingText = false;
    HitPart m_hover = HitPart::None;
    HitPart m_pressed = HitPart::None;
    float m_holdAcc = 0.0f;
    bool m_holdRepeat = false;
    AnimatedScalar m_hotUp{ 0.0f };
    AnimatedScalar m_hotDown{ 0.0f };
    Event<NumberBox*, float> m_onValueChangedEvent;
};

} // namespace CUI
