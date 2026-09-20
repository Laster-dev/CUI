#pragma once
#include "Button.h"

namespace CUI {

class ToggleButton : public Button {
public:
    ToggleButton();
    explicit ToggleButton(const std::string& text);
    virtual ~ToggleButton() = default;

    virtual const char* GetClassName() const override { return "ToggleButton"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void ApplyProperty(PropertyId id, const Value& val) override;

    virtual void OnMouseUp(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;

    PropertyRef<bool, PropertyId::IsOn> IsOn;

    bool GetIsChecked() const { return m_isChecked; }
    void ApplyIsChecked(bool checked);
    void ApplyChecked(bool checked) { ApplyIsChecked(checked); }

    /**
     * @brief 切换开关按钮选中状态布尔值属性代理。
     */
    struct ToggleButtonIsCheckedProperty {
        ToggleButton* owner;
        ToggleButtonIsCheckedProperty& operator=(bool c) { owner->ApplyIsChecked(c); return *this; }
        operator bool() const { return owner->GetIsChecked(); }
        bool Get() const { return owner->GetIsChecked(); }
        bool operator()() const { return owner->GetIsChecked(); }
    } IsChecked{this};

    Event<ToggleButton*, bool>& OnToggled() { return m_onToggledEvent; }

private:
    void ApplyCheckedChrome();
    void ToggleFromUser();

    bool m_isChecked = false;
    Event<ToggleButton*, bool> m_onToggledEvent;
};

} // namespace CUI
