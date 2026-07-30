#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ToggleSwitch.h"
#include <algorithm>
#include <cmath>

namespace CUI {

ToggleSwitch::ToggleSwitch() {
    SetProperty("isOn", Value(false));
    SetProperty("header", Value("开关 (ToggleSwitch)"));
    SetProperty("onColor", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("offColor", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
    SetProperty("knobColor", Value(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)));
    SetProperty("width", Value(180.0f));
    SetProperty("height", Value(24.0f));
}

std::vector<PropertyMeta> ToggleSwitch::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "header", "标题文本 (Header)", "开关配置", "string" });
    metas.push_back({ "isOn", "开启状态 (IsOn)", "开关配置", "bool" });
    metas.push_back({ "onColor", "开启颜色 (OnColor)", "色彩外观", "color" });
    metas.push_back({ "offColor", "关闭颜色 (OffColor)", "色彩外观", "color" });
    metas.push_back({ "knobColor", "滑块颜色 (KnobColor)", "色彩外观", "color" });
    return metas;
}

Size ToggleSwitch::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(180.0f);
    float expH = GetProperty("height").AsFloat(24.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ToggleSwitch::SetIsOn(bool on) {
    if (IsOn() != on) {
        SetProperty("isOn", Value(on));
        m_onToggledEvent.Invoke(this, on);
    }
}

bool ToggleSwitch::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    float targetRatio = IsOn() ? 1.0f : 0.0f;
    if (std::abs(m_knobPosRatio - targetRatio) > 0.01f) {
        m_knobPosRatio += (targetRatio - m_knobPosRatio) * 0.25f;
        return true;
    } else {
        m_knobPosRatio = targetRatio;
    }
    return base;
}

void ToggleSwitch::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    if (m_bounds.Contains(pt.x, pt.y)) {
        SetIsOn(!IsOn());
    }
}

void ToggleSwitch::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    float pillW = 40.0f;
    float pillH = 20.0f;
    Rect pillRect(m_bounds.x, m_bounds.y + (m_bounds.height - pillH) * 0.5f, pillW, pillH);

    D2D1_COLOR_F onColor = GetProperty("onColor").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));
    D2D1_COLOR_F offColor = GetProperty("offColor").AsColor(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f));
    D2D1_COLOR_F knobColor = GetProperty("knobColor").AsColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));

    // Linear interpolate background color
    D2D1_COLOR_F currentBg = D2D1::ColorF(
        offColor.r + (onColor.r - offColor.r) * m_knobPosRatio,
        offColor.g + (onColor.g - offColor.g) * m_knobPosRatio,
        offColor.b + (onColor.b - offColor.b) * m_knobPosRatio,
        1.0f
    );

    ctx.FillRoundedRect(pillRect, pillH * 0.5f, currentBg);

    // Render Sliding Circle Knob
    float knobRadius = 7.0f;
    float minX = pillRect.x + 3.0f + knobRadius;
    float maxX = pillRect.x + pillW - 3.0f - knobRadius;
    float knobCX = minX + (maxX - minX) * m_knobPosRatio;
    float knobCY = pillRect.y + pillH * 0.5f;

    Rect knobRect(knobCX - knobRadius, knobCY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
    ctx.FillRoundedRect(knobRect, knobRadius, knobColor);

    // Render Header Text
    std::string header = GetHeader();
    if (!header.empty()) {
        float fontSize = GetProperty("fontSize").AsFloat(13.0f);
        std::string fontFamily = GetProperty("fontFamily").AsString("Segoe UI");
        D2D1_COLOR_F textColor = GetProperty("color").AsColor(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f));

        Rect textRect(pillRect.x + pillW + 10.0f, m_bounds.y, (std::max)(0.0f, m_bounds.width - pillW - 10.0f), m_bounds.height);
        ctx.DrawText(header, textRect, textColor, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

} // namespace CUI
