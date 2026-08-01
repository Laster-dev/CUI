#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Slider.h"
#include <algorithm>
#include <cmath>

namespace CUI {
namespace {
float FrameBlend(float factorAt60Hz) {
    factorAt60Hz = std::clamp(factorAt60Hz, 0.0f, 0.999f);
    float frames = UIElement::GetAnimationDeltaSeconds() * 60.0f;
    return 1.0f - std::pow(1.0f - factorAt60Hz, (std::max)(0.1f, frames));
}
}

Slider::Slider() {
    SetProperty("minimum", Value(0.0f));
    SetProperty("maximum", Value(100.0f));
    SetProperty("value", Value(0.0f));
    SetProperty("step", Value(1.0f));
    SetProperty("orientation", Value("Horizontal"));
    SetProperty("trackColor", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
    SetProperty("activeTrackColor", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("thumbColor", Value(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)));
    SetProperty("width", Value(200.0f));
    SetProperty("height", Value(24.0f));
    m_displayValue = GetValue();
}

std::vector<PropertyMeta> Slider::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "value", "当前数值 (Value)", "滑块配置", "number" });
    metas.push_back({ "minimum", "最小值 (Minimum)", "滑块配置", "number" });
    metas.push_back({ "maximum", "最大值 (Maximum)", "滑块配置", "number" });
    metas.push_back({ "step", "步长 (Step)", "滑块配置", "number" });
    metas.push_back({ "orientation", "方向 (Orientation)", "滑块配置", "enum", { "Horizontal", "Vertical" } });
    metas.push_back({ "activeTrackColor", "激活轨颜色 (ActiveColor)", "色彩外观", "color" });
    metas.push_back({ "trackColor", "底轨颜色 (TrackColor)", "色彩外观", "color" });
    metas.push_back({ "thumbColor", "滑块颜色 (ThumbColor)", "色彩外观", "color" });
    return metas;
}

Size Slider::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(200.0f);
    float expH = GetProperty("height").AsFloat(24.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

Rect Slider::GetTrackRect() const {
    std::string orient = GetProperty("orientation").AsString("Horizontal");
    if (orient == "Horizontal") {
        float trackH = 4.0f;
        float y = m_bounds.y + (m_bounds.height - trackH) * 0.5f;
        return Rect(m_bounds.x + 8.0f, y, (std::max)(0.0f, m_bounds.width - 16.0f), trackH);
    } else {
        float trackW = 4.0f;
        float x = m_bounds.x + (m_bounds.width - trackW) * 0.5f;
        return Rect(x, m_bounds.y + 8.0f, trackW, (std::max)(0.0f, m_bounds.height - 16.0f));
    }
}

Rect Slider::GetThumbRect() const {
    Rect track = GetTrackRect();
    float minVal = GetMinimum();
    float maxVal = GetMaximum();
    float val = std::clamp(m_displayValue, minVal, maxVal);
    float ratio = (maxVal > minVal) ? (val - minVal) / (maxVal - minVal) : 0.0f;

    std::string orient = GetProperty("orientation").AsString("Horizontal");
    float thumbR = 8.0f;
    if (orient == "Horizontal") {
        float cx = track.x + ratio * track.width;
        float cy = track.y + track.height * 0.5f;
        return Rect(cx - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f);
    } else {
        float cx = track.x + track.width * 0.5f;
        float cy = (track.y + track.height) - ratio * track.height;
        return Rect(cx - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f);
    }
}

void Slider::MarkSliderVisualDirty(const Rect& previousThumb, float previousDisplayValue) {
    const Rect currentThumb = GetThumbRect();
    Rect dirty = previousThumb.Union(currentThumb).Inflate(4.0f);

    std::string orient = GetProperty("orientation").AsString("Horizontal");
    Rect track = GetTrackRect();
    float previousFillExtent = 0.0f;
    float currentFillExtent = 0.0f;
    if (orient == "Horizontal") {
        previousFillExtent = previousThumb.x + previousThumb.width * 0.5f;
        currentFillExtent = currentThumb.x + currentThumb.width * 0.5f;
        float left = (std::min)(previousFillExtent, currentFillExtent);
        float right = (std::max)(previousFillExtent, currentFillExtent);
        dirty = dirty.Union(Rect(track.x, track.y, (std::max)(0.0f, right - track.x), track.height).Inflate(2.0f));
        dirty = dirty.Union(Rect(left, track.y, (std::max)(0.0f, right - left), track.height).Inflate(2.0f));
    } else {
        previousFillExtent = previousThumb.y + previousThumb.height * 0.5f;
        currentFillExtent = currentThumb.y + currentThumb.height * 0.5f;
        float top = (std::min)(previousFillExtent, currentFillExtent);
        float bottom = (std::max)(previousFillExtent, currentFillExtent);
        dirty = dirty.Union(Rect(track.x, top, track.width, (std::max)(0.0f, bottom - top)).Inflate(2.0f));
        dirty = dirty.Union(track.Inflate(2.0f));
    }

    if (std::abs(previousDisplayValue - m_displayValue) > 0.01f) {
        dirty = dirty.Union(track.Inflate(2.0f));
    }

    MarkRenderRectDirty(dirty);
}

void Slider::SetValue(float val) {
    float minVal = GetMinimum();
    float maxVal = GetMaximum();
    float step = GetStep();
    if (step > 0.0f) {
        val = minVal + std::round((val - minVal) / step) * step;
    }
    val = std::clamp(val, minVal, maxVal);

    if (std::abs(GetValue() - val) > 0.0001f) {
        const Rect previousThumb = GetThumbRect();
        const float previousDisplayValue = m_displayValue;
        SetProperty("value", Value(val));
        if (m_isDragging) {
            m_displayValue = val;
        }
        MarkSliderVisualDirty(previousThumb, previousDisplayValue);
        m_onValueChangedEvent.Invoke(this, val);
    }
}

void Slider::UpdateValueFromPoint(Point pt) {
    Rect track = GetTrackRect();
    float minVal = GetMinimum();
    float maxVal = GetMaximum();
    std::string orient = GetProperty("orientation").AsString("Horizontal");

    float ratio = 0.0f;
    if (orient == "Horizontal") {
        if (track.width > 0.0f) {
            ratio = (pt.x - track.x) / track.width;
        }
    } else {
        if (track.height > 0.0f) {
            ratio = ((track.y + track.height) - pt.y) / track.height;
        }
    }
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    SetValue(minVal + ratio * (maxVal - minVal));
}

void Slider::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    m_isDragging = true;
    UpdateValueFromPoint(pt);
}

void Slider::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    if (m_isDragging) {
        UpdateValueFromPoint(pt);
    }
}

void Slider::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isDragging = false;
}

bool Slider::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    if (m_isDragging) {
        m_displayValue = GetValue();
        return base;
    }

    float target = GetValue();
    float delta = target - m_displayValue;
    if (std::abs(delta) <= 0.01f) {
        m_displayValue = target;
        return base;
    }
    const Rect previousThumb = GetThumbRect();
    const float previousDisplayValue = m_displayValue;
    m_displayValue += delta * FrameBlend(0.25f);
    MarkSliderVisualDirty(previousThumb, previousDisplayValue);
    return true;
}

bool Slider::HasSelfAnimation() const {
    if (m_isDragging) {
        return Control::HasSelfAnimation();
    }
    return Control::HasSelfAnimation() || std::abs(GetValue() - m_displayValue) > 0.01f;
}

void Slider::OnKeyDown(int vkCode) {
    Control::OnKeyDown(vkCode);
    float step = GetStep();
    if (vkCode == VK_LEFT || vkCode == VK_DOWN) {
        SetValue(GetValue() - step);
    } else if (vkCode == VK_RIGHT || vkCode == VK_UP) {
        SetValue(GetValue() + step);
    }
}

void Slider::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    Rect track = GetTrackRect();
    Rect thumb = GetThumbRect();
    std::string orient = GetProperty("orientation").AsString("Horizontal");
    D2D1_COLOR_F trackBg = GetProperty("trackColor").AsColor(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f));
    D2D1_COLOR_F activeBg = GetProperty("activeTrackColor").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));
    D2D1_COLOR_F thumbBg = GetProperty("thumbColor").AsColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));

    // Draw background track
    ctx.FillRoundedRect(track, 2.0f, trackBg);

    // Draw active track
    if (orient == "Horizontal") {
        float fillW = thumb.x + thumb.width * 0.5f - track.x;
        if (fillW > 0.0f) {
            ctx.FillRoundedRect(Rect(track.x, track.y, fillW, track.height), 2.0f, activeBg);
        }
    } else {
        float fillH = (track.y + track.height) - (thumb.y + thumb.height * 0.5f);
        if (fillH > 0.0f) {
            ctx.FillRoundedRect(Rect(track.x, thumb.y + thumb.height * 0.5f, track.width, fillH), 2.0f, activeBg);
        }
    }

    // Draw thumb circle
    ctx.FillRoundedRect(thumb, thumb.width * 0.5f, m_isHovered || m_isDragging ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : thumbBg);
    ctx.DrawRoundedRect(thumb, thumb.width * 0.5f, activeBg, 2.0f);
}

} // namespace CUI
