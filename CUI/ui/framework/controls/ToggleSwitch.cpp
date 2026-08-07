#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ToggleSwitch.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {
namespace {
// WinUI 3 ToggleSwitch uses ~167ms with a strong ease-out (fast start, soft settle).
constexpr float kToggleDurationSec = 0.167f;

float FluentEaseOut(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
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
    SetHeight(28.0f);
    m_knobPosAnim.Reset(0.0f);
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
    float expH = GetHeight(); if (expH < 0) expH = 28.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ToggleSwitch::SetIsOn(bool on) {
    if (m_isOn == on) {
        return;
    }
    m_isOn = on;
    NotifyFieldChanged(PropertyId::IsOn, Value(on));
    m_onToggledEvent.Invoke(this, on);
    m_knobPosAnim.SetTarget(on ? 1.0f : 0.0f);
    if (!UIElement::AreAnimationsEnabled()) {
        m_knobPosAnim.Reset(on ? 1.0f : 0.0f);
    } else {
        RequestAnimationTicks();
    }
    MarkRenderRectDirty(m_bounds);
}

bool ToggleSwitch::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    if (!UIElement::AreAnimationsEnabled()) {
        m_knobPosAnim.Reset(IsOn() ? 1.0f : 0.0f);
        return base;
    }

    // Time-based linear progress → single Fluent ease in paint (WinUI ToggleSwitch).
    const float target = IsOn() ? 1.0f : 0.0f;
    float current = m_knobPosAnim.Current();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    const float step = (dt <= 0.0f) ? 1.0f : (dt / kToggleDurationSec);
    if (current < target) {
        current = (std::min)(target, current + step);
    } else if (current > target) {
        current = (std::max)(target, current - step);
    }
    m_knobPosAnim.Reset(current);
    m_knobPosAnim.SetTarget(target);

    const bool moving = std::abs(current - target) > 0.001f;
    if (moving) {
        RequestAnimationTicks();
        MarkRenderRectDirty(m_bounds);
    }
    return base || moving;
}

bool ToggleSwitch::HasSelfAnimation() const {
    float targetRatio = IsOn() ? 1.0f : 0.0f;
    return Control::HasSelfAnimation() || std::abs(m_knobPosAnim.Current() - targetRatio) > 0.001f;
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

    // WinUI-ish pill proportions (~40x20 scaled down for dense gallery rows).
    float pillW = 40.0f;
    float pillH = 20.0f;
    Rect pillRect(m_bounds.x, m_bounds.y + (m_bounds.height - pillH) * 0.5f, pillW, pillH);

    D2D1_COLOR_F onColor = ResolveThemeColor(GetOnColorToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F offColor = ResolveThemeColor(GetOffColorToken(), ThemeTokenId::InputBorder);
    D2D1_COLOR_F knobColor = ResolveThemeColor(GetKnobColorToken(), ThemeTokenId::AccentForeground);
    D2D1_COLOR_F borderBrush = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    borderBrush.a = (std::min)(borderBrush.a, 0.55f);

    const float eased = FluentEaseOut(m_knobPosAnim.Current());
    D2D1_COLOR_F trackBg = BlendColor(offColor, onColor, eased);
    D2D1_COLOR_F trackBorder = BlendColor(borderBrush, onColor, eased * 0.9f);

    ctx.FillRoundedRect(pillRect, pillH * 0.5f, trackBg);
    ctx.DrawRoundedRect(pillRect, pillH * 0.5f, trackBorder, 1.0f);

    // Slight press scale via visual state (WinUI thumb grows a touch when pressed).
    const float press = m_visualStateAnim.Current();
    float knobRadius = 8.0f + 0.6f * (std::min)(1.0f, press);
    float minX = pillRect.x + 2.0f + knobRadius;
    float maxX = pillRect.x + pillW - 2.0f - knobRadius;
    float knobCX = minX + (maxX - minX) * eased;
    float knobCY = pillRect.y + pillH * 0.5f;

    Rect knobRect(knobCX - knobRadius, knobCY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
    ctx.FillRoundedRect(knobRect, knobRadius, knobColor);
    ctx.DrawRoundedRect(knobRect, knobRadius, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.12f), 1.0f);

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
