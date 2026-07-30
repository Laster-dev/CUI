#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ProgressBar.h"
#include <algorithm>
#include <cmath>

namespace CUI {

ProgressBar::ProgressBar() {
    SetProperty("minimum", Value(0.0f));
    SetProperty("maximum", Value(100.0f));
    SetProperty("value", Value(0.0f));
    SetProperty("isIndeterminate", Value(false));
    SetProperty("fillColor", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("trackColor", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
    SetProperty("width", Value(200.0f));
    SetProperty("height", Value(6.0f));
    SetProperty("cornerRadius", Value(3.0f));
    m_displayValue = GetValue();
}

std::vector<PropertyMeta> ProgressBar::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "value", "进度数值 (Value)", "进度配置", "number" });
    metas.push_back({ "minimum", "最小值 (Minimum)", "进度配置", "number" });
    metas.push_back({ "maximum", "最大值 (Maximum)", "进度配置", "number" });
    metas.push_back({ "isIndeterminate", "不确定模式 (IsIndeterminate)", "进度配置", "bool" });
    metas.push_back({ "fillColor", "填充颜色 (FillColor)", "色彩外观", "color" });
    metas.push_back({ "trackColor", "轨道颜色 (TrackColor)", "色彩外观", "color" });
    return metas;
}

Size ProgressBar::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(200.0f);
    float expH = GetProperty("height").AsFloat(6.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

bool ProgressBar::OnAnimationTick() {
    bool baseAnim = Control::OnAnimationTick();
    if (IsIndeterminate()) {
        m_animOffset += 4.0f;
        if (m_animOffset > m_bounds.width * 1.5f) {
            m_animOffset = -m_bounds.width * 0.5f;
        }
        return true;
    }

    float target = GetValue();
    float delta = target - m_displayValue;
    if (std::abs(delta) <= 0.01f) {
        m_displayValue = target;
        return baseAnim;
    }
    m_displayValue += delta * 0.25f;
    return true;
}

void ProgressBar::OnRender(GraphicsContext& ctx) {
    float radius = GetProperty("cornerRadius").AsFloat(3.0f);
    D2D1_COLOR_F trackBg = GetProperty("trackColor").AsColor(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f));
    D2D1_COLOR_F fillBg = GetProperty("fillColor").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));

    // Draw Track
    ctx.FillRoundedRect(m_bounds, radius, trackBg);

    if (IsIndeterminate()) {
        float chunkW = m_bounds.width * 0.4f;
        float chunkX = m_bounds.x + m_animOffset;
        Rect chunkRect(chunkX, m_bounds.y, chunkW, m_bounds.height);

        ctx.PushClip(m_bounds);
        ctx.FillRoundedRect(chunkRect, radius, fillBg);
        ctx.PopClip();
    } else {
        float minVal = GetMinimum();
        float maxVal = GetMaximum();
        float val = std::clamp(m_displayValue, minVal, maxVal);
        float ratio = (maxVal > minVal) ? (val - minVal) / (maxVal - minVal) : 0.0f;

        float fillW = m_bounds.width * ratio;
        if (fillW > 0.0f) {
            Rect fillRect(m_bounds.x, m_bounds.y, fillW, m_bounds.height);
            ctx.FillRoundedRect(fillRect, radius, fillBg);
        }
    }
}

} // namespace CUI
