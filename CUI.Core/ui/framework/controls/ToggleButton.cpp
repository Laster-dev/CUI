#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ToggleButton.h"
#include "../style/ThemeManager.h"

namespace CUI {

ToggleButton::ToggleButton() {
    SetText("Toggle");
    ApplyCheckedChrome();
}

ToggleButton::ToggleButton(const std::string& text) : ToggleButton() {
    SetText(text);
}

Value ToggleButton::GetProperty(PropertyId id) const {
    if (id == PropertyId::IsOn) {
        return Value(m_isChecked);
    }
    return Button::GetProperty(id);
}

bool ToggleButton::HasProperty(PropertyId id) const {
    if (id == PropertyId::IsOn) {
        return true;
    }
    return Button::HasProperty(id);
}

void ToggleButton::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::IsOn) {
        SetIsChecked(val.AsBool());
        return;
    }
    Button::SetProperty(id, val);
}

void ToggleButton::ApplyCheckedChrome() {
    if (m_isChecked) {
        SetBackgroundToken(ThemeTokenId::AccentColor);
        SetHoverBackgroundToken(ThemeTokenId::AccentColor);
        SetPressedBackgroundToken(ThemeTokenId::AccentColor);
        SetBorderToken(ThemeTokenId::AccentColor);
        SetColorToken(ThemeTokenId::AccentForeground);
        SetBorderThickness(0.0f);
    } else {
        SetBackgroundToken(ThemeTokenId::CardBackground);
        SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
        SetBorderToken(ThemeTokenId::CardBorder);
        SetColorToken(ThemeTokenId::TextPrimary);
        SetBorderThickness(1.0f);
    }
}

void ToggleButton::SetIsChecked(bool checked) {
    if (m_isChecked == checked) {
        return;
    }
    m_isChecked = checked;
    NotifyFieldChanged(PropertyId::IsOn, Value(checked));
    ApplyCheckedChrome();
    MarkRenderRectDirty(m_bounds);
    m_onToggledEvent.Invoke(this, m_isChecked);
}

void ToggleButton::ToggleFromUser() {
    SetIsChecked(!m_isChecked);
}

void ToggleButton::OnMouseUp(Point pt) {
    const bool fire = IsPressed() && IsEnabled() && m_bounds.Contains(pt.x, pt.y);
    Button::OnMouseUp(pt);
    if (fire) {
        ToggleFromUser();
    }
}

bool ToggleButton::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return false;
    }
    if (vkCode == VK_SPACE || vkCode == VK_RETURN) {
        ToggleFromUser();
        ExecuteBoundCommand();
        OnClick().Invoke(this);
        return true;
    }
    return Button::OnKeyDown(vkCode);
}

} // namespace CUI
