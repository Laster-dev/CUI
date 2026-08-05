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

    bool IsOn() const { return m_isOn; }
    void SetIsOn(bool on);

    const std::string& GetHeader() const { return m_header; }
    void SetHeader(const std::string& header) {
        m_header = header;
        MarkRenderContentDirty();
    }

    Event<ToggleSwitch*, bool>& OnToggled() { return m_onToggledEvent; }

private:
    bool m_isOn = false;
    std::string m_header{ "开关 (ToggleSwitch)" };
    AnimatedScalar m_knobPosAnim{}; // 0.0 for Off, 1.0 for On
    Event<ToggleSwitch*, bool> m_onToggledEvent;
};

} // namespace CUI
