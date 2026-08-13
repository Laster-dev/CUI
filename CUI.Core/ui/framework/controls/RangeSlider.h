#pragma once
#include "Control.h"

namespace CUI {

class RangeSlider : public Control {
public:
    RangeSlider();
    virtual ~RangeSlider() = default;

    virtual const char* GetClassName() const override { return "RangeSlider"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }
    bool AcceptsTabFocus() const override { return true; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    float GetMinimum() const { return m_minimum; }
    void SetMinimum(float minVal);
    float GetMaximum() const { return m_maximum; }
    void SetMaximum(float maxVal);
    float GetStep() const { return m_step; }
    void SetStep(float step);
    float GetMinimumRange() const { return m_minimumRange; }
    void SetMinimumRange(float range);

    float GetLowerValue() const { return m_lower; }
    void SetLowerValue(float val);
    float GetUpperValue() const { return m_upper; }
    void SetUpperValue(float val);
    void SetRange(float lower, float upper);

    Event<RangeSlider*, float, float>& OnValueChanged() { return m_onValueChanged; }

private:
    enum class Thumb { None, Lower, Upper };

    float Snap(float val) const;
    float ClampLower(float val) const;
    float ClampUpper(float val) const;
    float ValueFromPoint(Point pt) const;
    Rect GetTrackRect() const;
    Rect GetThumbRect(float displayValue) const;
    Rect GetLowerThumbRect() const;
    Rect GetUpperThumbRect() const;
    Rect GetFillRect() const;
    Thumb HitTestThumb(Point pt) const;
    Thumb CloserThumb(Point pt) const;
    void MarkRangeDirty(const Rect& prevLower, const Rect& prevUpper);
    void FireChanged();
    void DrawValueChip(GraphicsContext& ctx, const Rect& thumb, float value, bool active);

    float m_minimum = 0.0f;
    float m_maximum = 100.0f;
    float m_step = 1.0f;
    float m_minimumRange = 0.0f;
    float m_lower = 20.0f;
    float m_upper = 80.0f;
    AnimatedScalar m_lowerAnim{};
    AnimatedScalar m_upperAnim{};
    Thumb m_active = Thumb::Lower;
    Thumb m_hover = Thumb::None;
    bool m_dragging = false;
    Event<RangeSlider*, float, float> m_onValueChanged;
};

} // namespace CUI
