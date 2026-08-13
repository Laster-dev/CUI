#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "RangeSlider.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <windows.h>

namespace CUI {
namespace {

constexpr float kThumbR = 8.0f;
constexpr AnimationSpec kSlideSpec{ 0.25f, 0.01f };

std::string FormatRangeValue(float v) {
    if (std::abs(v - std::round(v)) < 0.001f) {
        return std::to_string(static_cast<int>(std::round(v)));
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    return buf;
}

} // namespace

RangeSlider::RangeSlider() {
    SetOrientation(Orientation::Horizontal);
    SetTrackColorToken(ThemeTokenId::InputBorder);
    SetActiveTrackColorToken(ThemeTokenId::AccentColor);
    SetThumbColorToken(ThemeTokenId::AccentColor);
    SetWidth(280.0f);
    SetHeight(48.0f);
    m_lowerAnim.Reset(m_lower);
    m_upperAnim.Reset(m_upper);
    SetKeyboardNavigationMode(KeyboardNavigationMode::Contained);
}

Value RangeSlider::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::LowerValue: return Value(m_lower);
    case PropertyId::UpperValue: return Value(m_upper);
    case PropertyId::Minimum: return Value(m_minimum);
    case PropertyId::Maximum: return Value(m_maximum);
    case PropertyId::Step: return Value(m_step);
    case PropertyId::MinimumRange: return Value(m_minimumRange);
    default: return Control::GetProperty(id);
    }
}

bool RangeSlider::HasProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::LowerValue:
    case PropertyId::UpperValue:
    case PropertyId::Minimum:
    case PropertyId::Maximum:
    case PropertyId::Step:
    case PropertyId::MinimumRange:
        return true;
    default:
        return Control::HasProperty(id);
    }
}

void RangeSlider::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::LowerValue: SetLowerValue(val.AsFloat()); return;
    case PropertyId::UpperValue: SetUpperValue(val.AsFloat()); return;
    case PropertyId::Minimum: SetMinimum(val.AsFloat()); return;
    case PropertyId::Maximum: SetMaximum(val.AsFloat()); return;
    case PropertyId::Step: SetStep(val.AsFloat()); return;
    case PropertyId::MinimumRange: SetMinimumRange(val.AsFloat()); return;
    default: UIElement::SetProperty(id, val); return;
    }
}

Size RangeSlider::Measure(Size availableSize) {
    (void)availableSize;
    float w = GetWidth();
    float h = GetHeight();
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    if (w < 0.0f) {
        w = horizontal ? 280.0f : 80.0f;
    }
    if (h < 0.0f) {
        h = horizontal ? 48.0f : 200.0f;
    }
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

float RangeSlider::Snap(float val) const {
    if (m_step > 0.0f) {
        val = m_minimum + std::round((val - m_minimum) / m_step) * m_step;
    }
    return std::clamp(val, m_minimum, m_maximum);
}

float RangeSlider::ClampLower(float val) const {
    val = Snap(val);
    const float maxLower = m_upper - m_minimumRange;
    return std::clamp(val, m_minimum, (std::max)(m_minimum, maxLower));
}

float RangeSlider::ClampUpper(float val) const {
    val = Snap(val);
    const float minUpper = m_lower + m_minimumRange;
    return std::clamp(val, (std::min)(m_maximum, minUpper), m_maximum);
}

void RangeSlider::SetMinimum(float minVal) {
    if (std::abs(m_minimum - minVal) < 0.0001f) {
        return;
    }
    m_minimum = minVal;
    if (m_maximum < m_minimum) {
        m_maximum = m_minimum;
    }
    NotifyFieldChanged(PropertyId::Minimum, Value(m_minimum));
    SetRange(m_lower, m_upper);
}

void RangeSlider::SetMaximum(float maxVal) {
    if (std::abs(m_maximum - maxVal) < 0.0001f) {
        return;
    }
    m_maximum = maxVal;
    if (m_minimum > m_maximum) {
        m_minimum = m_maximum;
    }
    NotifyFieldChanged(PropertyId::Maximum, Value(m_maximum));
    SetRange(m_lower, m_upper);
}

void RangeSlider::SetStep(float step) {
    step = (std::max)(0.0f, step);
    if (std::abs(m_step - step) < 0.0001f) {
        return;
    }
    m_step = step;
    NotifyFieldChanged(PropertyId::Step, Value(m_step));
    SetRange(m_lower, m_upper);
}

void RangeSlider::SetMinimumRange(float range) {
    range = (std::max)(0.0f, range);
    if (std::abs(m_minimumRange - range) < 0.0001f) {
        return;
    }
    m_minimumRange = range;
    NotifyFieldChanged(PropertyId::MinimumRange, Value(m_minimumRange));
    SetRange(m_lower, m_upper);
}

void RangeSlider::SetLowerValue(float val) {
    val = ClampLower(val);
    if (std::abs(m_lower - val) < 0.0001f) {
        return;
    }
    const Rect prev = GetLowerThumbRect();
    m_lower = val;
    if (m_dragging) {
        m_lowerAnim.Reset(m_lower);
    } else {
        NotifyFieldChanged(PropertyId::LowerValue, Value(m_lower));
        RequestAnimationTicks();
    }
    MarkThumbMoved(prev, GetLowerThumbRect());
    FireChanged();
}

void RangeSlider::SetUpperValue(float val) {
    val = ClampUpper(val);
    if (std::abs(m_upper - val) < 0.0001f) {
        return;
    }
    const Rect prev = GetUpperThumbRect();
    m_upper = val;
    if (m_dragging) {
        m_upperAnim.Reset(m_upper);
    } else {
        NotifyFieldChanged(PropertyId::UpperValue, Value(m_upper));
        RequestAnimationTicks();
    }
    MarkThumbMoved(prev, GetUpperThumbRect());
    FireChanged();
}

void RangeSlider::SetRange(float lower, float upper) {
    if (lower > upper) {
        std::swap(lower, upper);
    }
    lower = Snap(lower);
    upper = Snap(upper);
    lower = std::clamp(lower, m_minimum, m_maximum);
    upper = std::clamp(upper, m_minimum, m_maximum);
    if (upper - lower < m_minimumRange) {
        upper = std::clamp(lower + m_minimumRange, m_minimum, m_maximum);
        lower = std::clamp(upper - m_minimumRange, m_minimum, m_maximum);
    }
    const bool loChanged = std::abs(m_lower - lower) > 0.0001f;
    const bool hiChanged = std::abs(m_upper - upper) > 0.0001f;
    if (!loChanged && !hiChanged) {
        return;
    }
    const Rect prevL = GetLowerThumbRect();
    const Rect prevU = GetUpperThumbRect();
    m_lower = lower;
    m_upper = upper;
    if (m_dragging) {
        m_lowerAnim.Reset(m_lower);
        m_upperAnim.Reset(m_upper);
    } else {
        NotifyFieldChanged(PropertyId::LowerValue, Value(m_lower));
        NotifyFieldChanged(PropertyId::UpperValue, Value(m_upper));
        RequestAnimationTicks();
    }
    MarkBothThumbsDirty(prevL, prevU);
    FireChanged();
}

void RangeSlider::FlushPropertyNotify() {
    NotifyFieldChanged(PropertyId::LowerValue, Value(m_lower));
    NotifyFieldChanged(PropertyId::UpperValue, Value(m_upper));
}

void RangeSlider::FireChanged() {
    m_onValueChanged.Invoke(this, m_lower, m_upper);
}

Rect RangeSlider::GetTrackRect() const {
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    if (horizontal) {
        const float chipBand = (m_bounds.height >= 40.0f) ? 22.0f : 0.0f;
        const float trackH = 4.0f;
        const float y = m_bounds.y + chipBand + (m_bounds.height - chipBand - trackH) * 0.5f;
        return Rect(m_bounds.x + 8.0f, y, (std::max)(0.0f, m_bounds.width - 16.0f), trackH);
    }
    const float trackW = 4.0f;
    const float x = m_bounds.x + 14.0f;
    return Rect(x, m_bounds.y + 8.0f, trackW, (std::max)(0.0f, m_bounds.height - 16.0f));
}

Rect RangeSlider::GetThumbRect(float displayValue) const {
    const Rect track = GetTrackRect();
    const float span = m_maximum - m_minimum;
    const float ratio = span > 0.0f ? std::clamp((displayValue - m_minimum) / span, 0.0f, 1.0f) : 0.0f;
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    if (horizontal) {
        const float cx = track.x + ratio * track.width;
        const float cy = track.y + track.height * 0.5f;
        return Rect(cx - kThumbR, cy - kThumbR, kThumbR * 2.0f, kThumbR * 2.0f);
    }
    const float cx = track.x + track.width * 0.5f;
    const float cy = (track.y + track.height) - ratio * track.height;
    return Rect(cx - kThumbR, cy - kThumbR, kThumbR * 2.0f, kThumbR * 2.0f);
}

Rect RangeSlider::GetLowerThumbRect() const {
    return GetThumbRect(m_lowerAnim.Current());
}

Rect RangeSlider::GetUpperThumbRect() const {
    return GetThumbRect(m_upperAnim.Current());
}

Rect RangeSlider::GetFillRect() const {
    const Rect track = GetTrackRect();
    const Rect lo = GetLowerThumbRect();
    const Rect hi = GetUpperThumbRect();
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    if (horizontal) {
        const float x0 = lo.x + lo.width * 0.5f;
        const float x1 = hi.x + hi.width * 0.5f;
        return Rect(x0, track.y, (std::max)(0.0f, x1 - x0), track.height);
    }
    const float y0 = hi.y + hi.height * 0.5f;
    const float y1 = lo.y + lo.height * 0.5f;
    return Rect(track.x, y0, track.width, (std::max)(0.0f, y1 - y0));
}

Rect RangeSlider::ChipFootprint(const Rect& thumb) const {
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    if (horizontal) {
        return Rect(thumb.x + thumb.width * 0.5f - 28.0f, thumb.y - 28.0f, 56.0f, 28.0f);
    }
    return Rect(thumb.x + thumb.width, thumb.y - 4.0f, 48.0f, thumb.height + 8.0f);
}

void RangeSlider::MarkThumbMoved(const Rect& prevThumb, const Rect& currThumb) {
    Rect dirty = prevThumb.Union(currThumb).Inflate(2.0f);
    const Rect track = GetTrackRect();
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    if (horizontal) {
        const float a = prevThumb.x + prevThumb.width * 0.5f;
        const float b = currThumb.x + currThumb.width * 0.5f;
        const float left = (std::min)(a, b);
        const float right = (std::max)(a, b);
        dirty = dirty.Union(Rect(left, track.y, (std::max)(0.0f, right - left), track.height).Inflate(2.0f));
    } else {
        const float a = prevThumb.y + prevThumb.height * 0.5f;
        const float b = currThumb.y + currThumb.height * 0.5f;
        const float top = (std::min)(a, b);
        const float bottom = (std::max)(a, b);
        dirty = dirty.Union(Rect(track.x, top, track.width, (std::max)(0.0f, bottom - top)).Inflate(2.0f));
    }
    if (m_dragging || m_hover != Thumb::None || m_isFocused) {
        dirty = dirty.Union(ChipFootprint(prevThumb)).Union(ChipFootprint(currThumb));
    }
    MarkRenderRectDirty(dirty);
}

void RangeSlider::MarkBothThumbsDirty(const Rect& prevLower, const Rect& prevUpper) {
    MarkThumbMoved(prevLower, GetLowerThumbRect());
    MarkThumbMoved(prevUpper, GetUpperThumbRect());
}

float RangeSlider::ValueFromPoint(Point pt) const {
    const Rect track = GetTrackRect();
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    float ratio = 0.0f;
    if (horizontal) {
        if (track.width > 0.0f) {
            ratio = (pt.x - track.x) / track.width;
        }
    } else if (track.height > 0.0f) {
        ratio = ((track.y + track.height) - pt.y) / track.height;
    }
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    return m_minimum + ratio * (m_maximum - m_minimum);
}

RangeSlider::Thumb RangeSlider::HitTestThumb(Point pt) const {
    const Rect lo = GetLowerThumbRect().Inflate(4.0f);
    const Rect hi = GetUpperThumbRect().Inflate(4.0f);
    const bool hitLo = lo.Contains(pt.x, pt.y);
    const bool hitHi = hi.Contains(pt.x, pt.y);
    if (hitLo && hitHi) {
        return CloserThumb(pt);
    }
    if (hitLo) {
        return Thumb::Lower;
    }
    if (hitHi) {
        return Thumb::Upper;
    }
    return Thumb::None;
}

RangeSlider::Thumb RangeSlider::CloserThumb(Point pt) const {
    const float v = ValueFromPoint(pt);
    const float dLo = std::abs(v - m_lower);
    const float dHi = std::abs(v - m_upper);
    if (std::abs(dLo - dHi) < 0.0001f) {
        return m_active != Thumb::None ? m_active : Thumb::Lower;
    }
    return dLo <= dHi ? Thumb::Lower : Thumb::Upper;
}

void RangeSlider::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    // Skip Control::OnMouseDown — it RequestAnimationTicks a visual-state fade
    // that dirties the full bounds every frame while the thumb is already moving.
    UIElement::OnMouseDown(pt);
    Thumb hit = HitTestThumb(pt);
    if (hit == Thumb::None) {
        hit = CloserThumb(pt);
    }
    m_active = hit;
    m_dragging = true;
    const float v = ValueFromPoint(pt);
    if (m_active == Thumb::Upper) {
        SetUpperValue(v);
    } else {
        m_active = Thumb::Lower;
        SetLowerValue(v);
    }
}

void RangeSlider::OnMouseMove(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    UIElement::OnMouseMove(pt);
    if (m_dragging) {
        const float v = ValueFromPoint(pt);
        if (m_active == Thumb::Upper) {
            SetUpperValue(v);
        } else {
            SetLowerValue(v);
        }
        return;
    }
    const Thumb hover = HitTestThumb(pt);
    if (hover != m_hover) {
        const Rect prevL = GetLowerThumbRect();
        const Rect prevU = GetUpperThumbRect();
        m_hover = hover;
        MarkRenderRectDirty(prevL.Union(ChipFootprint(prevL)).Union(prevU).Union(ChipFootprint(prevU)).Inflate(2.0f));
    }
}

void RangeSlider::OnMouseUp(Point pt) {
    UIElement::OnMouseUp(pt);
    const bool wasDragging = m_dragging;
    m_dragging = false;
    m_hover = HitTestThumb(pt);
    if (wasDragging) {
        FlushPropertyNotify();
    }
    if (std::abs(m_lower - m_lowerAnim.Current()) > 0.01f
        || std::abs(m_upper - m_upperAnim.Current()) > 0.01f) {
        RequestAnimationTicks();
    }
}

void RangeSlider::OnMouseLeave() {
    UIElement::OnMouseLeave();
    if (m_hover != Thumb::None) {
        const Rect lo = GetLowerThumbRect();
        const Rect hi = GetUpperThumbRect();
        m_hover = Thumb::None;
        MarkRenderRectDirty(lo.Union(ChipFootprint(lo)).Union(hi).Union(ChipFootprint(hi)).Inflate(2.0f));
    }
}

bool RangeSlider::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return false;
    }
    if (vkCode == 'L' || vkCode == VK_OEM_4) {
        m_active = Thumb::Lower;
        MarkRenderRectDirty(m_bounds);
        return true;
    }
    if (vkCode == 'U' || vkCode == VK_OEM_6) {
        m_active = Thumb::Upper;
        MarkRenderRectDirty(m_bounds);
        return true;
    }
    const float step = m_step > 0.0f ? m_step : 1.0f;
    float delta = 0.0f;
    if (vkCode == VK_LEFT || vkCode == VK_DOWN) {
        delta = -step;
    } else if (vkCode == VK_RIGHT || vkCode == VK_UP) {
        delta = step;
    } else if (vkCode == VK_PRIOR) {
        delta = step * 10.0f;
    } else if (vkCode == VK_NEXT) {
        delta = -step * 10.0f;
    } else if (vkCode == VK_HOME) {
        if (m_active == Thumb::Upper) {
            SetUpperValue(m_lower + m_minimumRange);
        } else {
            SetLowerValue(m_minimum);
        }
        return true;
    } else if (vkCode == VK_END) {
        if (m_active == Thumb::Upper) {
            SetUpperValue(m_maximum);
        } else {
            SetLowerValue(m_upper - m_minimumRange);
        }
        return true;
    }
    if (delta == 0.0f) {
        return false;
    }
    if (m_active == Thumb::Upper) {
        SetUpperValue(m_upper + delta);
    } else {
        SetLowerValue(m_lower + delta);
    }
    return true;
}

bool RangeSlider::OnAnimationTick() {
    if (m_dragging) {
        m_lowerAnim.Reset(m_lower);
        m_upperAnim.Reset(m_upper);
        return false;
    }
    bool base = Control::OnAnimationTick();
    m_lowerAnim.SetTarget(m_lower);
    m_upperAnim.SetTarget(m_upper);
    const bool moving = m_lowerAnim.IsAnimating(0.01f) || m_upperAnim.IsAnimating(0.01f);
    if (!moving) {
        m_lowerAnim.Reset(m_lower);
        m_upperAnim.Reset(m_upper);
        return base;
    }
    const Rect prevL = GetLowerThumbRect();
    const Rect prevU = GetUpperThumbRect();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    m_lowerAnim.Tick(dt, kSlideSpec);
    m_upperAnim.Tick(dt, kSlideSpec);
    MarkBothThumbsDirty(prevL, prevU);
    return true;
}

bool RangeSlider::HasSelfAnimation() const {
    if (m_dragging) {
        return false;
    }
    return Control::HasSelfAnimation()
        || std::abs(m_lower - m_lowerAnim.Current()) > 0.01f
        || std::abs(m_upper - m_upperAnim.Current()) > 0.01f;
}

void RangeSlider::DrawValueChip(GraphicsContext& ctx, const Rect& thumb, float value, bool active) {
    const auto accent = ResolveThemeColor(GetActiveTrackColorToken(), ThemeTokenId::AccentColor);
    const auto onAccent = ResolveThemeColor(ThemeTokenId::AccentForeground, ThemeTokenId::AccentForeground);
    const std::string label = FormatRangeValue(value);
    const float w = (std::max)(28.0f, static_cast<float>(label.size()) * 7.5f + 12.0f);
    const float h = 20.0f;
    const bool horizontal = GetOrientation() != Orientation::Vertical;
    Rect chip;
    if (horizontal) {
        chip = Rect(thumb.x + thumb.width * 0.5f - w * 0.5f, thumb.y - h - 6.0f, w, h);
    } else {
        chip = Rect(thumb.x + thumb.width + 8.0f, thumb.y + thumb.height * 0.5f - h * 0.5f, w, h);
    }
    const float alpha = active ? 1.0f : 0.82f;
    ctx.FillRoundedRect(chip, 4.0f, D2D1::ColorF(accent.r, accent.g, accent.b, alpha));
    ctx.DrawText(
        label,
        chip,
        onAccent,
        "微软雅黑",
        11.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
        DWRITE_FONT_WEIGHT_SEMI_BOLD);
}

void RangeSlider::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const Rect track = GetTrackRect();
    const Rect lo = GetLowerThumbRect();
    const Rect hi = GetUpperThumbRect();
    const Rect fill = GetFillRect();
    const auto trackBg = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::InputBorder);
    const auto activeBg = ResolveThemeColor(GetActiveTrackColorToken(), ThemeTokenId::AccentColor);
    const auto thumbFill = ResolveThemeColor(GetThumbColorToken(), ThemeTokenId::AccentColor);

    ctx.FillRoundedRect(track, 2.0f, trackBg);
    if (fill.width > 0.0f && fill.height > 0.0f) {
        ctx.FillRoundedRect(fill, 2.0f, activeBg);
    }

    auto paintThumb = [&](const Rect& thumb, Thumb which) {
        const bool hot = m_active == which || m_hover == which || (m_dragging && m_active == which);
        const Rect r = hot ? thumb.Inflate(1.0f) : thumb;
        ctx.FillRoundedRect(r, r.width * 0.5f, thumbFill);
    };
    paintThumb(lo, Thumb::Lower);
    paintThumb(hi, Thumb::Upper);

    if (m_dragging || m_hover != Thumb::None || m_isFocused) {
        DrawValueChip(ctx, lo, m_lowerAnim.Current(), m_active == Thumb::Lower);
        DrawValueChip(ctx, hi, m_upperAnim.Current(), m_active == Thumb::Upper);
    }
}

} // namespace CUI
