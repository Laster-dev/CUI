#pragma once
#include "Control.h"

namespace CUI {

class ToggleSwitch : public Control {
public:
    ToggleSwitch();
    virtual ~ToggleSwitch() = default;

    virtual const char* GetClassName() const override { return "ToggleSwitch"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual void OnFocus() override;
    virtual void OnBlur() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    bool IsOn() const { return m_isOn; }
    void SetIsOn(bool on);

    const std::string& GetHeader() const { return m_header; }
    void SetHeader(const std::string& header) {
        if (m_header == header) return;
        m_header = header;
        NotifyFieldChanged(PropertyId::Header, Value(header));
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
