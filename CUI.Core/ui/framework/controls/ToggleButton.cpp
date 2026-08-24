#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ToggleButton.h"
#include "../core/CUIDsl.h"
#include "../style/ThemeManager.h"

namespace CUI {

ToggleButton::ToggleButton() {
    IsOn.Initialize(*this);
    DSL::Borrow(this).Text("Toggle");
    ApplyCheckedChrome();
}

ToggleButton::ToggleButton(const std::string& text) : ToggleButton() {
    DSL::Borrow(this).Text(text);
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
        DSL::Borrow(this)
            .BackgroundToken(ThemeTokenId::AccentColor)
            .HoverBackgroundToken(ThemeTokenId::AccentColor)
            .PressedBackgroundToken(ThemeTokenId::AccentColor)
            .BorderToken(ThemeTokenId::AccentColor)
            .ForegroundToken(ThemeTokenId::AccentForeground)
            .Background(theme.GetColor("accentColor"))
            .HoverBackground(theme.GetColor("accentColor"))
            .PressedBackground(theme.GetColor("accentColor"))
            .BorderBrush(theme.GetColor("accentColor"))
            .Foreground(theme.GetColor("accentForeground"))
            .BorderThickness(0.0f);
    } else {
        DSL::Borrow(this)
            .BackgroundToken(ThemeTokenId::CardBackground)
            .HoverBackgroundToken(ThemeTokenId::HoverBackground)
            .PressedBackgroundToken(ThemeTokenId::PressedBackground)
            .BorderToken(ThemeTokenId::CardBorder)
            .ForegroundToken(ThemeTokenId::TextPrimary)
            .Background(theme.GetColor("cardBackground"))
            .HoverBackground(theme.GetColor("hoverBackground"))
            .PressedBackground(theme.GetColor("pressedBackground"))
            .BorderBrush(theme.GetColor("cardBorder"))
            .Foreground(theme.GetColor("textPrimary"))
            .BorderThickness(1.0f);
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
