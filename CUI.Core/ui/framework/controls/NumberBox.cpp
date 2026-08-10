#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "NumberBox.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace CUI {

NumberBox::NumberBox() {
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    SetWidth(120.0f);
    SetHeight(28.0f);

    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetBorderToken(ThemeTokenId::InputBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetBackground(tokens.inputBackground);
    SetBorderBrush(tokens.inputBorder);
    SetColor(tokens.textPrimary);
    SetBorderThickness(1.0f);

    m_inputBox = std::make_shared<TextBox>();
    m_inputBox->SetText("0");
    m_inputBox->SetBackgroundToken(ThemeTokenId::InputBackground);
    m_inputBox->SetBorderToken(ThemeTokenId::InputBorder);
    m_inputBox->SetColorToken(ThemeTokenId::TextPrimary);
    m_inputBox->SetBackground(tokens.inputBackground);
    m_inputBox->SetBorderBrush(tokens.inputBorder);
    m_inputBox->SetColor(tokens.textPrimary);
    m_inputBox->SetBorderThickness(0.0f);
    m_inputBox->SetCornerRadius(0.0f);

    m_btnUp = std::make_shared<Button>("▲");
    m_btnUp->SetFontSize(8.0f);
    m_btnUp->SetBackgroundToken(ThemeTokenId::CardBackground);
    m_btnUp->SetHoverBackgroundToken(ThemeTokenId::CardBackground);
    m_btnUp->SetBorderToken(ThemeTokenId::CardBorder);
    m_btnUp->SetColorToken(ThemeTokenId::TextSecondary);
    m_btnUp->SetBackground(tokens.cardBackground);
    m_btnUp->SetHoverBackground(tokens.cardBackground);
    m_btnUp->SetBorderBrush(tokens.cardBorder);
    m_btnUp->SetColor(tokens.textSecondary);
    m_btnUp->SetPadding(Thickness(0));
    m_btnUp->SetBorderThickness(0.0f);
    m_btnUp->SetCornerRadius(0.0f);

    m_btnDown = std::make_shared<Button>("▼");
    m_btnDown->SetFontSize(8.0f);
    m_btnDown->SetBackgroundToken(ThemeTokenId::CardBackground);
    m_btnDown->SetHoverBackgroundToken(ThemeTokenId::CardBackground);
    m_btnDown->SetBorderToken(ThemeTokenId::CardBorder);
    m_btnDown->SetColorToken(ThemeTokenId::TextSecondary);
    m_btnDown->SetBackground(tokens.cardBackground);
    m_btnDown->SetHoverBackground(tokens.cardBackground);
    m_btnDown->SetBorderBrush(tokens.cardBorder);
    m_btnDown->SetColor(tokens.textSecondary);
    m_btnDown->SetPadding(Thickness(0));
    m_btnDown->SetBorderThickness(0.0f);
    m_btnDown->SetCornerRadius(0.0f);

    AddChild(m_inputBox);
    AddChild(m_btnUp);
    AddChild(m_btnDown);

    m_btnUp->OnClick().Connect([this](UIElement*) {
        SetValue(GetValue() + GetStep());
    });

    m_btnDown->OnClick().Connect([this](UIElement*) {
        SetValue(GetValue() - GetStep());
    });
}

std::vector<PropertyMeta> NumberBox::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "value", "当前数值 (Value)", "数值配置", "number" });
    metas.push_back({ "minimum", "最小值 (Minimum)", "数值配置", "number" });
    metas.push_back({ "maximum", "最大值 (Maximum)", "数值配置", "number" });
    metas.push_back({ "step", "步进 (Step)", "数值配置", "number" });
    return metas;
}

Size NumberBox::Measure(Size availableSize) {
    (void)availableSize;
    float expW = GetWidth(); if (expW < 0) expW = 120.0f;
    float expH = GetHeight(); if (expH < 0) expH = 28.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void NumberBox::Arrange(Rect finalRect) {
    m_bounds = finalRect;
    float borderThick = GetBorderThickness();
    float innerX = finalRect.x + borderThick;
    float innerY = finalRect.y + borderThick;
    float innerW = (std::max)(0.0f, finalRect.width - borderThick * 2.0f);
    float innerH = (std::max)(0.0f, finalRect.height - borderThick * 2.0f);

    float btnW = 18.0f;
    float btnH = innerH * 0.5f;

    m_inputBox->Arrange(Rect(innerX, innerY, (std::max)(0.0f, innerW - btnW), innerH));
    m_btnUp->Arrange(Rect(innerX + innerW - btnW, innerY, btnW, btnH));
    m_btnDown->Arrange(Rect(innerX + innerW - btnW, innerY + btnH, btnW, btnH));
}

void NumberBox::SetValue(float val) {
    float minV = GetMinimum();
    float maxV = GetMaximum();
    float clamped = std::clamp(val, minV, maxV);
    m_value = clamped;
    NotifyFieldChanged(PropertyId::ControlValue, Value(clamped));

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << clamped;
    std::string s = ss.str();
    if (s.size() > 2 && s.substr(s.size() - 2) == ".0") {
        s = s.substr(0, s.size() - 2);
    }
    m_inputBox->SetText(s);
    m_onValueChangedEvent.Invoke(this, clamped);
    MarkRenderContentDirty();
}

void NumberBox::OnKeyDown(int vkCode) {
    if (vkCode == VK_UP) {
        SetValue(GetValue() + GetStep());
        return;
    }
    if (vkCode == VK_DOWN) {
        SetValue(GetValue() - GetStep());
        return;
    }

    Control::OnKeyDown(vkCode);
}

} // namespace CUI
