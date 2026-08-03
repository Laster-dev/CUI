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
    SetProperty("value", Value(0.0f));
    SetProperty("minimum", Value(-100000.0f));
    SetProperty("maximum", Value(100000.0f));
    SetProperty("step", Value(1.0f));
    SetProperty("width", Value(120.0f));
    SetProperty("height", Value(28.0f));

    SetProperty("theme.backgroundToken", Value("inputBackground"));
    SetProperty("theme.borderToken", Value("inputBorder"));
    SetProperty("theme.colorToken", Value("textPrimary"));
    SetProperty("background", Value(tokens.inputBackground));
    SetProperty("borderBrush", Value(tokens.inputBorder));
    SetProperty("color", Value(tokens.textPrimary));
    SetProperty("borderThickness", Value(1.0f));

    m_inputBox = std::make_shared<TextBox>();
    m_inputBox->SetText("0");
    m_inputBox->SetProperty("theme.backgroundToken", Value("inputBackground"));
    m_inputBox->SetProperty("theme.borderToken", Value("inputBorder"));
    m_inputBox->SetProperty("theme.colorToken", Value("textPrimary"));
    m_inputBox->SetProperty("background", Value(tokens.inputBackground));
    m_inputBox->SetProperty("borderBrush", Value(tokens.inputBorder));
    m_inputBox->SetProperty("color", Value(tokens.textPrimary));
    m_inputBox->SetProperty("borderThickness", Value(0.0f));
    m_inputBox->SetProperty("cornerRadius", Value(0.0f));

    m_btnUp = std::make_shared<Button>("▲");
    m_btnUp->SetProperty("fontSize", Value(8.0f));
    m_btnUp->SetProperty("theme.backgroundToken", Value("cardBackground"));
    m_btnUp->SetProperty("theme.hoverBackgroundToken", Value("cardBackground"));
    m_btnUp->SetProperty("theme.borderToken", Value("cardBorder"));
    m_btnUp->SetProperty("theme.colorToken", Value("textSecondary"));
    m_btnUp->SetProperty("background", Value(tokens.cardBackground));
    m_btnUp->SetProperty("hoverBackground", Value(tokens.cardBackground));
    m_btnUp->SetProperty("borderBrush", Value(tokens.cardBorder));
    m_btnUp->SetProperty("color", Value(tokens.textSecondary));
    m_btnUp->SetProperty("padding", Value(Thickness(0)));
    m_btnUp->SetProperty("borderThickness", Value(0.0f));
    m_btnUp->SetProperty("cornerRadius", Value(0.0f));

    m_btnDown = std::make_shared<Button>("▼");
    m_btnDown->SetProperty("fontSize", Value(8.0f));
    m_btnDown->SetProperty("theme.backgroundToken", Value("cardBackground"));
    m_btnDown->SetProperty("theme.hoverBackgroundToken", Value("cardBackground"));
    m_btnDown->SetProperty("theme.borderToken", Value("cardBorder"));
    m_btnDown->SetProperty("theme.colorToken", Value("textSecondary"));
    m_btnDown->SetProperty("background", Value(tokens.cardBackground));
    m_btnDown->SetProperty("hoverBackground", Value(tokens.cardBackground));
    m_btnDown->SetProperty("borderBrush", Value(tokens.cardBorder));
    m_btnDown->SetProperty("color", Value(tokens.textSecondary));
    m_btnDown->SetProperty("padding", Value(Thickness(0)));
    m_btnDown->SetProperty("borderThickness", Value(0.0f));
    m_btnDown->SetProperty("cornerRadius", Value(0.0f));

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
    float expW = GetProperty("width").AsFloat(120.0f);
    float expH = GetProperty("height").AsFloat(28.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void NumberBox::Arrange(Rect finalRect) {
    m_bounds = finalRect;
    float borderThick = GetProperty("borderThickness").AsFloat(1.0f);
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
    float minV = GetProperty("minimum").AsFloat(-100000.0f);
    float maxV = GetProperty("maximum").AsFloat(100000.0f);
    float clamped = std::clamp(val, minV, maxV);
    SetProperty("value", Value(clamped));

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
