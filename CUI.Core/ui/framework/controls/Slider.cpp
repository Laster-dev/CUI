#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Slider.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {
namespace {}

Slider::Slider() {
    ValueProperty.Initialize(*this);
    SetOrientation(Orientation::Horizontal);
    SetTrackColorToken(ThemeTokenId::InputBorder);
    SetActiveTrackColorToken(ThemeTokenId::AccentColor);
    SetThumbColorToken(ThemeTokenId::AccentColor);
    SetWidth(200.0f);
    SetHeight(24.0f);
    m_displayValueAnim.Reset(GetValue());
    SetKeyboardNavigationMode(KeyboardNavigationMode::Contained);
}

Value Slider::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::ControlValue: return Value(m_value);
    case PropertyId::Minimum: return Value(m_minimum);
    case PropertyId::Maximum: return Value(m_maximum);
    case PropertyId::Step: return Value(m_step);
    default: return UIElement::GetProperty(id);
    }
}

bool Slider::HasProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::ControlValue:
    case PropertyId::Minimum:
    case PropertyId::Maximum:
    case PropertyId::Step:
        return true;
    default:
        return UIElement::HasProperty(id);
    }
}

void Slider::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::ControlValue: SetValue(val.AsFloat()); return;
    case PropertyId::Minimum: SetMinimum(val.AsFloat()); return;
    case PropertyId::Maximum: SetMaximum(val.AsFloat()); return;
    case PropertyId::Step: SetStep(val.AsFloat()); return;
    default: UIElement::SetProperty(id, val); return;
    }
}

Size Slider::Measure(Size availableSize) {
    (void)availableSize;
    float expW = GetWidth(); if (expW < 0) expW = 200.0f;
    float expH = GetHeight(); if (expH < 0) expH = 24.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

Rect Slider::GetTrackRect() const {
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    if (horizontal) {
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
    float val = std::clamp(m_displayValueAnim.Current(), minVal, maxVal);
    float ratio = (maxVal > minVal) ? (val - minVal) / (maxVal - minVal) : 0.0f;

    const bool horizontal = GetOrientation() != Orientation::Vertical;
    float thumbR = 8.0f;
    if (horizontal) {
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

    const bool horizontal = GetOrientation() != Orientation::Vertical;
    Rect track = GetTrackRect();
    float previousFillExtent = 0.0f;
    float currentFillExtent = 0.0f;
    if (horizontal) {
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

    if (std::abs(previousDisplayValue - m_displayValueAnim.Current()) > 0.01f) {
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
        const float previousDisplayValue = m_displayValueAnim.Current();
        m_value = val;
        NotifyFieldChanged(PropertyId::ControlValue, Value(val));
        if (m_isDragging) {
            m_displayValueAnim.Reset(val);
        }
        MarkSliderVisualDirty(previousThumb, previousDisplayValue);
        m_onValueChangedEvent.Invoke(this, val);
    }
}

void Slider::UpdateValueFromPoint(Point pt) {
    Rect track = GetTrackRect();
    float minVal = GetMinimum();
    float maxVal = GetMaximum();
    bool horizontal = GetOrientation() != Orientation::Vertical;

    float ratio = 0.0f;
    if (horizontal) {
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
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseDown(pt);
    m_isDragging = true;
    UpdateValueFromPoint(pt);
}

void Slider::OnMouseMove(Point pt) {
    if (!IsEnabled()) {
        return;
    }
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
        m_displayValueAnim.Reset(GetValue());
        return base;
    }

    float target = GetValue();
    m_displayValueAnim.SetTarget(target);
    if (!m_displayValueAnim.IsAnimating(0.01f)) {
        m_displayValueAnim.Reset(target);
        return base;
    }
    const Rect previousThumb = GetThumbRect();
    const float previousDisplayValue = m_displayValueAnim.Current();
    m_displayValueAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.25f, 0.01f });
    MarkSliderVisualDirty(previousThumb, previousDisplayValue);
    return true;
}

bool Slider::HasSelfAnimation() const {
    if (m_isDragging) {
        return Control::HasSelfAnimation();
    }
    return Control::HasSelfAnimation() || std::abs(GetValue() - m_displayValueAnim.Current()) > 0.01f;
}

bool Slider::OnKeyDown(int vkCode) {
    float step = GetStep();
    if (vkCode == VK_LEFT || vkCode == VK_DOWN) {
        SetValue(GetValue() - step);
        return true;
    }
    if (vkCode == VK_RIGHT || vkCode == VK_UP) {
        SetValue(GetValue() + step);
        return true;
    }
    return Control::OnKeyDown(vkCode);
}

void Slider::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    Rect track = GetTrackRect();
    Rect thumb = GetThumbRect();
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    D2D1_COLOR_F trackBg = m_hasBorderBrushColor
        ? m_borderBrushColor
        : ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::InputBorder);
    D2D1_COLOR_F activeBg = m_hasBackgroundColor
        ? m_backgroundColor
        : ResolveThemeColor(GetActiveTrackColorToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F thumbFill = ResolveThemeColor(GetThumbColorToken(), ThemeTokenId::CardBackground);

    // Draw background track
    ctx.FillRoundedRect(track, 2.0f, trackBg);

    // Draw active track
    if (horizontal) {
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

    // Draw WinUI 3 style thumb circle: Card background (white in light mode) with Accent border ring
    ctx.FillRoundedRect(thumb, thumb.width * 0.5f, thumbFill);
    ctx.DrawRoundedRect(thumb, thumb.width * 0.5f, activeBg, m_isHovered || m_isDragging ? 2.5f : 2.0f);

    // Center accent dot
    float dotRadius = (m_isHovered || m_isDragging) ? 3.5f : 2.5f;
    Point center(thumb.x + thumb.width * 0.5f, thumb.y + thumb.height * 0.5f);
    Rect dotRect(center.x - dotRadius, center.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    ctx.FillRoundedRect(dotRect, dotRadius, activeBg);
}

} // namespace CUI
