#pragma once
#include "Control.h"
#include <cstdint>

namespace CUI {

class NumberBox : public Control {
public:
    NumberBox();
    virtual ~NumberBox() = default;

    virtual const char* GetClassName() const override { return "NumberBox"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override;
    bool AcceptsTabFocus() const override { return true; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseWheel(float delta) override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnCharInput(wchar_t ch) override;
    virtual void OnFocus() override;
    virtual void OnBlur() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

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
    enum class HitPart : uint8_t { None, Text, Up, Down };

    static constexpr float kSpinnerW = 18.0f;

    Rect SpinnerCol() const;
    Rect UpBtn() const;
    Rect DownBtn() const;
    Rect TextRect() const;
    HitPart HitTestPart(Point pt) const;
    std::string FormatValue(float val) const;
    void SyncTextFromValue();
    void CommitEdit();
    void StepBy(float dir);
    void SetCaret(int pos);
    void InsertChar(char ch);
    void DeleteBackward();
    void DeleteForward();

    float m_value = 0.0f;
    float m_minimum = -100000.0f;
    float m_maximum = 100000.0f;
    float m_step = 1.0f;
    std::string m_editText = "0";
    int m_caret = 1;
    bool m_caretVisible = true;
    float m_caretBlink = 0.0f;
    HitPart m_hover = HitPart::None;
    HitPart m_pressed = HitPart::None;
    float m_holdAcc = 0.0f;
    bool m_holdRepeat = false;
    AnimatedScalar m_hotUp{ 0.0f };
    AnimatedScalar m_hotDown{ 0.0f };
    Event<NumberBox*, float> m_onValueChangedEvent;
};

} // namespace CUI
