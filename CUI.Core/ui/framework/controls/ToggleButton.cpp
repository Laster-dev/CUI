#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ToggleButton.h"
#include "../core/CUIDsl.h"
#include "../style/ThemeManager.h"

namespace CUI {

ToggleButton::ToggleButton() {
    IsOn.Initialize(*this);
    this->SetText("Toggle");
    ApplyCheckedChrome();
}

ToggleButton::ToggleButton(const std::string& text) : ToggleButton() {
    this->SetText(text);
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
    ThemeManager& theme = ThemeManager::Instance();
    if (m_isChecked) {
                this->SetBackgroundToken(ThemeTokenId::AccentColor);
        this->SetHoverBackgroundToken(ThemeTokenId::AccentColor);
        this->SetPressedBackgroundToken(ThemeTokenId::AccentColor);
        this->SetBorderToken(ThemeTokenId::AccentColor);
        this->SetBackground(theme.GetColor("accentColor"));
        this->SetHoverBackground(theme.GetColor("accentColor"));
        this->SetPressedBackground(theme.GetColor("accentColor"));
        this->SetBorderBrush(theme.GetColor("accentColor"));
        this->SetColor(theme.GetColor("accentForeground"));
        this->SetBorderThickness(0.0f);
    } else {
                this->SetBackgroundToken(ThemeTokenId::CardBackground);
        this->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        this->SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
        this->SetBorderToken(ThemeTokenId::CardBorder);
        this->SetBackground(theme.GetColor("cardBackground"));
        this->SetHoverBackground(theme.GetColor("hoverBackground"));
        this->SetPressedBackground(theme.GetColor("pressedBackground"));
        this->SetBorderBrush(theme.GetColor("cardBorder"));
        this->SetColor(theme.GetColor("textPrimary"));
        this->SetBorderThickness(1.0f);
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
