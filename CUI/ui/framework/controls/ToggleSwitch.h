#pragma once
#include "Control.h"

namespace CUI {

class ToggleSwitch : public Control {
public:
    ToggleSwitch();
    virtual ~ToggleSwitch() = default;

    virtual const char* GetClassName() const override { return "ToggleSwitch"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    bool IsOn() const { return GetProperty("isOn").AsBool(false); }
    void SetIsOn(bool on);

    std::string GetHeader() const { return GetProperty("header").AsString(""); }
    void SetHeader(const std::string& header) { SetProperty("header", Value(header)); }

    Event<ToggleSwitch*, bool>& OnToggled() { return m_onToggledEvent; }

private:
    AnimatedScalar m_knobPosAnim{}; // 0.0 for Off, 1.0 for On
    Event<ToggleSwitch*, bool> m_onToggledEvent;
};

} // namespace CUI
