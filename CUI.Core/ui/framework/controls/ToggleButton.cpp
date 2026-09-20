#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ToggleButton.h"
#include "../core/CUIDsl.h"
#include "../style/ThemeManager.h"

namespace CUI {

ToggleButton::ToggleButton() {
    IsOn.Initialize(*this);
    this->ApplyText("Toggle");
    ApplyCheckedChrome();
}

ToggleButton::ToggleButton(const std::string& text) : ToggleButton() {
    this->ApplyText(text);
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

void ToggleButton::ApplyProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::IsOn) {
        ApplyIsChecked(val.AsBool());
        return;
    }
    Button::ApplyProperty(id, val);
}

void ToggleButton::ApplyCheckedChrome() {
    ThemeManager& theme = ThemeManager::Instance();
    if (m_isChecked) {
                this->ApplyBackgroundToken(ThemeTokenId::AccentColor);
        this->ApplyHoverBackgroundToken(ThemeTokenId::AccentColor);
        this->ApplyPressedBackgroundToken(ThemeTokenId::AccentColor);
        this->ApplyBorderToken(ThemeTokenId::AccentColor);
        this->ApplyBackground(theme.GetColor("accentColor"));
        this->ApplyHoverBackground(theme.GetColor("accentColor"));
        this->ApplyPressedBackground(theme.GetColor("accentColor"));
        this->ApplyBorderBrush(theme.GetColor("accentColor"));
        this->ApplyColor(theme.GetColor("accentForeground"));
        this->ApplyBorderThickness(0.0f);
    } else {
                this->ApplyBackgroundToken(ThemeTokenId::CardBackground);
        this->ApplyHoverBackgroundToken(ThemeTokenId::HoverBackground);
        this->ApplyPressedBackgroundToken(ThemeTokenId::PressedBackground);
        this->ApplyBorderToken(ThemeTokenId::CardBorder);
        this->ApplyBackground(theme.GetColor("cardBackground"));
        this->ApplyHoverBackground(theme.GetColor("hoverBackground"));
        this->ApplyPressedBackground(theme.GetColor("pressedBackground"));
        this->ApplyBorderBrush(theme.GetColor("cardBorder"));
        this->ApplyColor(theme.GetColor("textPrimary"));
        this->ApplyBorderThickness(1.0f);
    }
}

void ToggleButton::ApplyIsChecked(bool checked) {
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
    ApplyIsChecked(!m_isChecked);
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
