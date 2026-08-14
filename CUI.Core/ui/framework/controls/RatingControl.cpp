#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "RatingControl.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kPadX = 4.0f;
constexpr float kPadY = 4.0f;
constexpr float kStarGap = 6.0f;
constexpr float kClearSlot = 18.0f;
constexpr float kInnerRatio = 0.382f;
constexpr AnimationSpec kFillSpec{ 0.22f, 0.01f, 0.16f };

D2D1_COLOR_F WithAlpha(D2D1_COLOR_F c, float a) {
    c.a = std::clamp(a, 0.0f, 1.0f);
    return c;
}

void StarPoints(Point center, float radius, Point out[10]) {
    const float inner = radius * kInnerRatio;
    for (int i = 0; i < 10; ++i) {
        const float ang = -kPi * 0.5f + static_cast<float>(i) * kPi * 0.2f;
        const float r = (i % 2 == 0) ? radius : inner;
        out[i] = Point(center.x + std::cos(ang) * r, center.y + std::sin(ang) * r);
    }
}

} // namespace

RatingControl::RatingControl() {
    ValueProperty.Initialize(*this);
    SetFillColorToken(ThemeTokenId::AccentColor);
    SetTrackColorToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextSecondary);
    SetBackground(D2D1::ColorF(0, 0, 0, 0));
    SetHoverBackground(D2D1::ColorF(0, 0, 0, 0));
    SetPressedBackground(D2D1::ColorF(0, 0, 0, 0));
    SetWidth(-1.0f);
    SetHeight(32.0f);
    m_displayValueAnim.Reset(m_value);
}

HCURSOR RatingControl::GetCursor() const {
    return CanInteract() ? LoadCursor(nullptr, IDC_HAND) : nullptr;
}

Value RatingControl::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::ControlValue: return Value(m_value);
    case PropertyId::Maximum: return Value(static_cast<float>(m_maxRating));
    case PropertyId::Step: return Value(m_step);
    case PropertyId::IsReadOnly: return Value(m_isReadOnly);
    case PropertyId::IsClearEnabled: return Value(m_isClearEnabled);
    default: return Control::GetProperty(id);
    }
}

bool RatingControl::HasProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::ControlValue:
    case PropertyId::Maximum:
    case PropertyId::Step:
    case PropertyId::IsReadOnly:
    case PropertyId::IsClearEnabled:
        return true;
    default:
        return Control::HasProperty(id);
    }
}

void RatingControl::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::ControlValue: SetValue(val.AsFloat()); return;
    case PropertyId::Maximum: SetMaxRating(static_cast<int>(val.AsFloat(static_cast<float>(m_maxRating)))); return;
    case PropertyId::Step: SetStep(val.AsFloat(m_step)); return;
    case PropertyId::IsReadOnly: SetIsReadOnly(val.AsBool()); return;
    case PropertyId::IsClearEnabled: SetIsClearEnabled(val.AsBool()); return;
    default: Control::SetProperty(id, val); return;
    }
}

bool RatingControl::CanInteract() const {
    return IsEnabled() && !m_isReadOnly;
}

float RatingControl::SnapValue(float val) const {
    const float maxV = static_cast<float>(m_maxRating);
    val = std::clamp(val, 0.0f, maxV);
    const float step = (m_step > 0.0f) ? m_step : 0.5f;
    val = std::round(val / step) * step;
    if (val < 0.0f) {
        val = 0.0f;
    }
    if (val > maxV) {
        val = maxV;
    }
    return val;
}

float RatingControl::ShownValue() const {
    if (m_hoverValue >= 0.0f) {
        return m_hoverValue;
    }
    return m_displayValueAnim.Current();
}

Rect RatingControl::ContentRect() const {
    return Rect(
        m_bounds.x + kPadX,
        m_bounds.y + kPadY,
        (std::max)(0.0f, m_bounds.width - kPadX * 2.0f),
        (std::max)(0.0f, m_bounds.height - kPadY * 2.0f));
}

Rect RatingControl::ClearRect() const {
    if (!m_isClearEnabled) {
        return Rect();
    }
    const Rect content = ContentRect();
    const float size = (std::min)(kClearSlot, content.height);
    return Rect(
        content.x,
        content.y + (content.height - size) * 0.5f,
        size,
        size);
}

Rect RatingControl::StarRect(int index) const {
    const Rect content = ContentRect();
    float x = content.x;
    if (m_isClearEnabled) {
        x += kClearSlot + kStarGap;
    }
    x += static_cast<float>(index) * (m_starSize + kStarGap);
    const float y = content.y + (content.height - m_starSize) * 0.5f;
    return Rect(x, y, m_starSize, m_starSize);
}

float RatingControl::ValueFromPoint(Point pt) const {
    if (m_isClearEnabled && ClearRect().Contains(pt.x, pt.y)) {
        return 0.0f;
    }
    for (int i = 0; i < m_maxRating; ++i) {
        const Rect star = StarRect(i);
        const float left = (i == 0) ? star.x : (star.x - kStarGap * 0.5f);
        const float right = star.x + star.width + kStarGap * 0.5f;
        if (pt.x < left || pt.x >= right) {
            continue;
        }
        const float local = std::clamp((pt.x - star.x) / (std::max)(star.width, 1.0f), 0.0f, 1.0f);
        const float full = static_cast<float>(i + 1);
        if (m_step <= 0.51f) {
            return SnapValue(local < 0.5f ? full - 0.5f : full);
        }
        return SnapValue(full);
    }
    if (pt.x < StarRect(0).x) {
        return 0.0f;
    }
    return static_cast<float>(m_maxRating);
}

Size RatingControl::Measure(Size availableSize) {
    (void)availableSize;
    float w = GetWidth();
    const float starsW = static_cast<float>(m_maxRating) * m_starSize
        + static_cast<float>((std::max)(0, m_maxRating - 1)) * kStarGap;
    const float clearW = m_isClearEnabled ? (kClearSlot + kStarGap) : 0.0f;
    if (w < 0.0f) {
        w = kPadX * 2.0f + clearW + starsW;
    }
    float h = GetHeight();
    if (h < 0.0f) {
        h = m_starSize + kPadY * 2.0f;
    }
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void RatingControl::SetValue(float val) {
    val = SnapValue(val);
    if (std::abs(m_value - val) <= 0.0001f) {
        return;
    }
    m_value = val;
    NotifyFieldChanged(PropertyId::ControlValue, Value(val));
    if (m_isDragging || m_hoverValue >= 0.0f || !UIElement::AreAnimationsEnabled()) {
        m_displayValueAnim.Reset(val);
    } else {
        m_displayValueAnim.SetTarget(val);
        RequestAnimationTicks();
    }
    MarkRenderRectDirty(m_bounds);
    m_onValueChangedEvent.Invoke(this, val);
}

void RatingControl::SetMaxRating(int maxRating) {
    maxRating = std::clamp(maxRating, 1, 10);
    if (m_maxRating == maxRating) {
        return;
    }
    m_maxRating = maxRating;
    NotifyFieldChanged(PropertyId::Maximum, Value(static_cast<float>(maxRating)));
    InvalidateMeasure();
    SetValue(m_value);
    MarkRenderRectDirty(m_bounds);
}

void RatingControl::SetStep(float step) {
    step = (step <= 0.0f) ? 0.5f : step;
    if (m_step == step) {
        return;
    }
    m_step = step;
    NotifyFieldChanged(PropertyId::Step, Value(step));
    SetValue(m_value);
}

void RatingControl::SetIsReadOnly(bool readOnly) {
    if (m_isReadOnly == readOnly) {
        return;
    }
    m_isReadOnly = readOnly;
    NotifyFieldChanged(PropertyId::IsReadOnly, Value(readOnly));
    if (readOnly) {
        m_hoverValue = -1.0f;
        m_isDragging = false;
    }
    MarkRenderRectDirty(m_bounds);
}

void RatingControl::SetIsClearEnabled(bool enabled) {
    if (m_isClearEnabled == enabled) {
        return;
    }
    m_isClearEnabled = enabled;
    NotifyFieldChanged(PropertyId::IsClearEnabled, Value(enabled));
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void RatingControl::SetStarSize(float size) {
    size = std::clamp(size, 12.0f, 48.0f);
    if (m_starSize == size) {
        return;
    }
    m_starSize = size;
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void RatingControl::CommitFromPoint(Point pt) {
    const float next = ValueFromPoint(pt);
    if (m_isClearEnabled && std::abs(next - m_value) <= 0.0001f) {
        SetValue(0.0f);
        return;
    }
    SetValue(next);
}

void RatingControl::OnMouseDown(Point pt) {
    if (!CanInteract()) {
        return;
    }
    Control::OnMouseDown(pt);
    m_isDragging = true;
    m_hoverValue = ValueFromPoint(pt);
    CommitFromPoint(pt);
}

void RatingControl::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    if (!CanInteract()) {
        return;
    }
    const float nextHover = ValueFromPoint(pt);
    if (m_isDragging) {
        if (std::abs(nextHover - m_hoverValue) > 0.0001f) {
            m_hoverValue = nextHover;
        }
        SetValue(nextHover);
        return;
    }
    if (std::abs(nextHover - m_hoverValue) > 0.0001f) {
        m_hoverValue = nextHover;
        MarkRenderRectDirty(m_bounds);
    }
}

void RatingControl::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isDragging = false;
    if (CanInteract()) {
        m_hoverValue = ValueFromPoint(pt);
        MarkRenderRectDirty(m_bounds);
    }
}

void RatingControl::OnMouseLeave() {
    Control::OnMouseLeave();
    m_isDragging = false;
    if (m_hoverValue >= 0.0f) {
        m_hoverValue = -1.0f;
        MarkRenderRectDirty(m_bounds);
    }
}

bool RatingControl::OnKeyDown(int vkCode) {
    if (!CanInteract()) {
        return false;
    }
    const float step = (m_step > 0.0f) ? m_step : 0.5f;
    if (vkCode == VK_LEFT || vkCode == VK_DOWN) {
        SetValue(m_value - step);
        return true;
    }
    if (vkCode == VK_RIGHT || vkCode == VK_UP) {
        SetValue(m_value + step);
        return true;
    }
    if (vkCode == VK_HOME) {
        SetValue(m_isClearEnabled ? 0.0f : step);
        return true;
    }
    if (vkCode == VK_END) {
        SetValue(static_cast<float>(m_maxRating));
        return true;
    }
    if (m_isClearEnabled && (vkCode == VK_DELETE || vkCode == VK_BACK)) {
        SetValue(0.0f);
        return true;
    }
    return Control::OnKeyDown(vkCode);
}

bool RatingControl::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    if (m_isDragging || m_hoverValue >= 0.0f) {
        m_displayValueAnim.Reset(m_value);
        return base;
    }
    m_displayValueAnim.SetTarget(m_value);
    if (!m_displayValueAnim.IsAnimating(kFillSpec.epsilon)) {
        m_displayValueAnim.Reset(m_value);
        return base;
    }
    const float prev = m_displayValueAnim.Current();
    m_displayValueAnim.Tick(UIElement::GetAnimationDeltaSeconds(), kFillSpec);
    if (std::abs(m_displayValueAnim.Current() - prev) > 0.001f) {
        MarkRenderRectDirty(m_bounds);
    }
    return true;
}

bool RatingControl::HasSelfAnimation() const {
    if (m_isDragging || m_hoverValue >= 0.0f) {
        return Control::HasSelfAnimation();
    }
    return Control::HasSelfAnimation()
        || std::abs(m_value - m_displayValueAnim.Current()) > kFillSpec.epsilon;
}

void RatingControl::DrawStar(
    GraphicsContext& ctx,
    const Rect& slot,
    float fill01,
    D2D1_COLOR_F fill,
    D2D1_COLOR_F empty,
    D2D1_COLOR_F stroke) const
{
    const Point center(slot.x + slot.width * 0.5f, slot.y + slot.height * 0.5f);
    const float radius = (std::min)(slot.width, slot.height) * 0.48f;
    Point pts[10];
    StarPoints(center, radius, pts);

    ctx.FillPolygon(pts, 10, empty);
    ctx.DrawPolygon(pts, 10, stroke, 1.0f);

    fill01 = std::clamp(fill01, 0.0f, 1.0f);
    if (fill01 <= 0.001f) {
        return;
    }
    if (fill01 >= 0.999f) {
        ctx.FillPolygon(pts, 10, fill);
        return;
    }
    const Rect clip(slot.x, slot.y, slot.width * fill01, slot.height);
    ctx.PushClip(clip);
    ctx.FillPolygon(pts, 10, fill);
    ctx.PopClip();
}

void RatingControl::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const D2D1_COLOR_F fill = ResolveThemeColor(GetFillColorToken(), ThemeTokenId::AccentColor);
    D2D1_COLOR_F empty = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::CardBorder);
    empty.a *= 0.35f;
    D2D1_COLOR_F stroke = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::CardBorder);
    stroke.a = (std::min)(1.0f, stroke.a * 0.85f);
    const D2D1_COLOR_F glyph = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);

    if (m_isClearEnabled) {
        const Rect clear = ClearRect();
        const float inset = clear.width * 0.28f;
        D2D1_COLOR_F xColor = glyph;
        if (m_hoverValue == 0.0f && CanInteract()) {
            xColor = fill;
        }
        ctx.DrawSmoothLine(
            Point(clear.x + inset, clear.y + inset),
            Point(clear.x + clear.width - inset, clear.y + clear.height - inset),
            xColor, 1.6f);
        ctx.DrawSmoothLine(
            Point(clear.x + clear.width - inset, clear.y + inset),
            Point(clear.x + inset, clear.y + clear.height - inset),
            xColor, 1.6f);
    }

    const float shown = ShownValue();
    for (int i = 0; i < m_maxRating; ++i) {
        const float starFill = std::clamp(shown - static_cast<float>(i), 0.0f, 1.0f);
        DrawStar(ctx, StarRect(i), starFill, fill, empty, stroke);
    }

    if (IsFocused() && CanInteract()) {
        ctx.DrawRoundedRect(m_bounds, 4.0f, WithAlpha(fill, 0.85f), 1.5f);
    }
}

} // namespace CUI
