#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ToggleSwitch.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {
namespace {
float EaseTrack(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f - std::pow(1.0f - t, 2.4f);
}
}

ToggleSwitch::ToggleSwitch() {
    auto& theme = ThemeManager::Instance();
    SetOnColorToken(ThemeTokenId::AccentColor);
    SetOffColorToken(ThemeTokenId::InputBorder);
    SetKnobColorToken(ThemeTokenId::AccentForeground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextSecondary);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    SetBorderBrush(theme.GetColor("cardBorder"));
    SetColor(theme.GetColor("textSecondary"));
    SetHoverBackground(theme.GetColor("hoverBackground"));
    SetPressedBackground(theme.GetColor("pressedBackground"));
    SetWidth(170.0f);
    SetHeight(24.0f);
}

std::vector<PropertyMeta> ToggleSwitch::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "header", "标题文本 (Header)", "开关配置", "string" });
    metas.push_back({ "isOn", "开启状态 (IsOn)", "开关配置", "bool" });
    return metas;
}

Size ToggleSwitch::Measure(Size availableSize) {
    (void)availableSize;
    float expW = GetWidth(); if (expW < 0) expW = 180.0f;
    float expH = GetHeight(); if (expH < 0) expH = 24.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ToggleSwitch::SetIsOn(bool on) {
    if (m_isOn != on) {
        m_isOn = on;
        NotifyFieldChanged(PropertyId::IsOn, Value(on));
        m_onToggledEvent.Invoke(this, on);
    }
}

bool ToggleSwitch::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    float targetRatio = IsOn() ? 1.0f : 0.0f;
    m_knobPosAnim.SetTarget(targetRatio);
    return m_knobPosAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.18f, 0.01f }) || base;
}

bool ToggleSwitch::HasSelfAnimation() const {
    float targetRatio = IsOn() ? 1.0f : 0.0f;
    return Control::HasSelfAnimation() || std::abs(m_knobPosAnim.Current() - targetRatio) > 0.01f;
}

void ToggleSwitch::OnMouseUp(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseUp(pt);
    if (m_bounds.Contains(pt.x, pt.y)) {
        SetIsOn(!IsOn());
    }
}

void ToggleSwitch::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    float pillW = 30.0f;
    float pillH = 17.0f;
    Rect pillRect(m_bounds.x, m_bounds.y + (m_bounds.height - pillH) * 0.5f, pillW, pillH);

    D2D1_COLOR_F onColor = ResolveThemeColor(GetOnColorToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F offColor = ResolveThemeColor(GetOffColorToken(), ThemeTokenId::InputBorder);
    D2D1_COLOR_F knobColor = ResolveThemeColor(GetKnobColorToken(), ThemeTokenId::AccentForeground);
    D2D1_COLOR_F borderBrush = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    borderBrush.a = (std::min)(borderBrush.a, 0.75f);

    float eased = EaseTrack(m_knobPosAnim.Current());
    D2D1_COLOR_F trackBg = BlendColor(offColor, onColor, eased);
    D2D1_COLOR_F trackBorder = BlendColor(borderBrush, onColor, eased * 0.85f);

    ctx.FillRoundedRect(pillRect, pillH * 0.5f, trackBg);
    ctx.DrawRoundedRect(pillRect, pillH * 0.5f, trackBorder, 1.0f);

    if (eased > 0.01f) {
        float inset = 2.0f;
        float activeWidth = (pillRect.width - inset * 2.0f) * eased;
        float activeX = pillRect.x + (pillRect.width - activeWidth) * 0.5f;
        D2D1_COLOR_F activeGlow = D2D1::ColorF(onColor.r, onColor.g, onColor.b, 0.18f + 0.14f * eased);
        ctx.FillRoundedRect(Rect(activeX, pillRect.y + inset, activeWidth, pillRect.height - inset * 2.0f), (pillRect.height - inset * 2.0f) * 0.5f, activeGlow);
    }

    float knobRadius = 6.5f + 0.25f * m_visualStateAnim.Current();
    float minX = pillRect.x + 3.0f + knobRadius;
    float maxX = pillRect.x + pillW - 3.0f - knobRadius;
    float knobCX = minX + (maxX - minX) * eased;
    float knobCY = pillRect.y + pillH * 0.5f;

    Rect knobRect(knobCX - knobRadius, knobCY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
    ctx.FillRoundedRect(knobRect, knobRadius, knobColor);
    ctx.DrawRoundedRect(knobRect, knobRadius, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.16f), 1.0f);

    std::string header = GetHeader();
    if (!header.empty()) {
        float fontSize = GetFontSize();
        const std::string& fontFamily = GetFontFamily();
        D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);

        Rect textRect(pillRect.x + pillW + 10.0f, m_bounds.y, (std::max)(0.0f, m_bounds.width - pillW - 10.0f), m_bounds.height);
        ctx.DrawText(header, textRect, textColor, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

} // namespace CUI
