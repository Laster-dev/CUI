#pragma once
#include "Button.h"

namespace CUI {

class ToggleButton : public Button {
public:
    ToggleButton();
    explicit ToggleButton(const std::string& text);
    virtual ~ToggleButton() = default;

    virtual const char* GetClassName() const override { return "ToggleButton"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;

    virtual void OnMouseUp(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;

    bool IsChecked() const { return m_isChecked; }
    void SetIsChecked(bool checked);

    Event<ToggleButton*, bool>& OnToggled() { return m_onToggledEvent; }

private:
    void ApplyCheckedChrome();
    void ToggleFromUser();

    bool m_isChecked = false;
    Event<ToggleButton*, bool> m_onToggledEvent;
};

} // namespace CUI
