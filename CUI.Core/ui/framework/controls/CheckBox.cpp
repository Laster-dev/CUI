#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "CheckBox.h"
#include "../style/ThemeManager.h"
#include <windows.h>
#include <algorithm>
#include <cmath>

namespace CUI {
namespace {
float EaseOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

float EaseOutQuad(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f - (1.0f - t) * (1.0f - t);
}

Point LerpPoint(const Point& a, const Point& b, float t) {
    return Point(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

}

CheckBox::CheckBox() {
    Checked.Initialize(*this);
    State.Initialize(*this);
    SetText("CheckBox");
    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetCheckedBackgroundToken(ThemeTokenId::AccentColor);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetBackground(ThemeManager::Instance().GetColor("inputBackground"));
    SetColor(ThemeManager::Instance().GetColor("textPrimary"));
    SetFontSize(12.0f);
    SetFontFamily("微软雅黑");
    SetPadding(Thickness(4, 4, 4, 4));
    SetCornerRadius(3.0f);
}

CheckBox::CheckBox(const std::string& text) : CheckBox() {
    SetText(text);
}

CheckBox::~CheckBox() {
    Unbind();
}

Value CheckBox::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::CheckState:
        if (m_state == CheckState::Checked) return Value("Checked");
        if (m_state == CheckState::Indeterminate) return Value("Indeterminate");
        return Value("Unchecked");
    case PropertyId::ControlValue: return Value(m_state == CheckState::Checked);
    case PropertyId::IsThreeState: return Value(m_isThreeState);
    default: return Control::GetProperty(id);
    }
}

bool CheckBox::HasProperty(PropertyId id) const {
    return id == PropertyId::CheckState || id == PropertyId::ControlValue || id == PropertyId::IsThreeState || Control::HasProperty(id);
}

void CheckBox::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::CheckState: {
        const std::string s = val.AsString("Unchecked");
        SetState(s == "Checked" ? CheckState::Checked
            : (s == "Indeterminate" ? CheckState::Indeterminate : CheckState::Unchecked));
        return;
    }
    case PropertyId::ControlValue: SetState(val.AsBool() ? CheckState::Checked : CheckState::Unchecked); return;
    case PropertyId::IsThreeState: SetIsThreeState(val.AsBool()); return;
    default: Control::SetProperty(id, val); return;
    }
}

void CheckBox::SetState(CheckState state) {
    if (m_state == state) {
        return;
    }
    std::string s = "Unchecked";
    if (state == CheckState::Checked) s = "Checked";
    else if (state == CheckState::Indeterminate) s = "Indeterminate";

    m_state = state;
    NotifyFieldChanged(PropertyId::CheckState, Value(s));
    NotifyFieldChanged(PropertyId::ControlValue, Value(state == CheckState::Checked));
    MarkRenderRectDirty(m_bounds);
    RequestAnimationTicks();
    m_onCheckStateChangedEvent.Invoke(this, state);
}

void CheckBox::Bind(const std::shared_ptr<Observable<bool>>& value) {
    Checked.Bind(value);
}

void CheckBox::Bind(const std::shared_ptr<Observable<CheckState>>& value, bool twoWay) {
    State.Bind(value, twoWay ? BindingMode::TwoWay : BindingMode::OneWay);
}

void CheckBox::Unbind() {
    Checked.Unbind();
    State.Unbind();
}

Size CheckBox::Measure(Size availableSize) {
    (void)availableSize;
    const std::string& text = GetText();
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();

    GraphicsContext ctx;
    Size textSize = ctx.MeasureText(text, font, fontSize);

    Thickness margin = GetMargin();
    Thickness padding = GetPadding();

    float boxW = 16.0f;
    float gap = 8.0f;

    float w = boxW + gap + textSize.width + margin.left + margin.right + padding.left + padding.right;
    float h = (std::max)(16.0f, textSize.height) + margin.top + margin.bottom + padding.top + padding.bottom;

    float expW = GetWidth();
    float expH = GetHeight();

    if (expW >= 0.0f) w = expW;
    if (expH >= 0.0f) h = expH;

    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void CheckBox::OnRender(GraphicsContext& ctx) {
    Thickness padding = GetPadding();
    float radius = GetCornerRadius();

    CheckState state = GetState();
    float fillTarget = state == CheckState::Unchecked ? 0.0f : 1.0f;
    float checkTarget = state == CheckState::Checked ? 1.0f : 0.0f;
    float indeterminateTarget = state == CheckState::Indeterminate ? 1.0f : 0.0f;
    m_fillAnim.SetTarget(fillTarget);
    m_checkAnim.SetTarget(checkTarget);
    m_indeterminateAnim.SetTarget(indeterminateTarget);

    if (!UIElement::AreAnimationsEnabled()) {
        m_fillAnim.Reset(fillTarget);
        m_checkAnim.Reset(checkTarget);
        m_indeterminateAnim.Reset(indeterminateTarget);
        UpdateVisualStateTarget();
        m_visualStateAnim.Reset(m_visualStateTarget);
    }

    float boxSize = 18.0f;
    float boxY = m_bounds.y + (m_bounds.height - boxSize) / 2.0f;
    Rect boxRect(m_bounds.x + padding.left, boxY, boxSize, boxSize);

    D2D1_COLOR_F accentBase = ResolveThemeColor(GetCheckedBackgroundToken(), ThemeTokenId::AccentColor);
    float visualState = m_visualStateAnim.Current();
    D2D1_COLOR_F accentBlue = BlendColor(accentBase, ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentColor), visualState * 0.35f);
    D2D1_COLOR_F checkedIconColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentForeground);
    D2D1_COLOR_F bg = GetAnimatedBackground(ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::InputBackground));
    D2D1_COLOR_F border = BlendColor(
        ThemeManager::Instance().GetFlatColor(ThemeTokenId::InputBorder),
        ThemeManager::Instance().GetFlatColor(ThemeTokenId::FocusedBorder),
        (std::min)(1.0f, visualState / 0.55f));

    float fillProgress = m_fillAnim.Current();
    float checkProgress = m_checkAnim.Current();
    float indeterminateProgress = m_indeterminateAnim.Current();

    D2D1_COLOR_F fillColor = BlendColor(bg, accentBlue, fillProgress);
    D2D1_COLOR_F borderColor = BlendColor(border, accentBlue, fillProgress * 0.9f);
    ctx.FillRoundedRect(boxRect, radius, fillColor);
    ctx.DrawRoundedRect(boxRect, radius, borderColor, 1.2f);

    float checkFactor = EaseOutCubic(checkProgress);
    if (checkFactor > 0.001f) {
        Point p1(boxRect.x + 4.6f, boxRect.y + 9.4f);
        Point p2(boxRect.x + 7.6f, boxRect.y + 12.3f);
        Point p3(boxRect.x + 13.4f, boxRect.y + 6.0f);
        float len1 = std::sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
        float len2 = std::sqrt((p3.x - p2.x) * (p3.x - p2.x) + (p3.y - p2.y) * (p3.y - p2.y));
        float total = len1 + len2;
        float drawLen = total * checkFactor;

        if (drawLen <= len1) {
            float segT = len1 > 0.0f ? drawLen / len1 : 1.0f;
            ctx.DrawLine(p1, LerpPoint(p1, p2, segT), checkedIconColor, 1.9f);
        } else {
            ctx.DrawLine(p1, p2, checkedIconColor, 1.9f);
            float remain = drawLen - len1;
            float segT = len2 > 0.0f ? remain / len2 : 1.0f;
            ctx.DrawLine(p2, LerpPoint(p2, p3, segT), checkedIconColor, 1.9f);
        }
    }

    float indeterminateFactor = EaseOutQuad(indeterminateProgress);
    if (indeterminateFactor > 0.001f) {
        float halfWidth = 4.5f * indeterminateFactor;
        float centerX = boxRect.x + boxRect.width * 0.5f;
        float centerY = boxRect.y + boxRect.height * 0.5f;
        Rect barRect(centerX - halfWidth, centerY - 1.5f, halfWidth * 2.0f, 3.0f);
        ctx.FillRoundedRect(barRect, 1.0f, checkedIconColor);
    }

    // Draw Label Text
    const std::string& text = GetText();
    if (!text.empty()) {
        float textX = boxRect.x + boxSize + 10.0f;
        Rect textRect(textX, m_bounds.y, m_bounds.width - (textX - m_bounds.x), m_bounds.height);

        D2D1_COLOR_F textColor = m_hasColorValue
            ? m_colorValue
            : ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
        const std::string& font = GetFontFamily();
        float fontSize = GetFontSize();

        ctx.DrawText(text, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER, ResolveFontWeight(), false, ResolveFontStyle(), ResolveFontStretch(), IsUnderline(), IsStrikethrough());
    }
}

void CheckBox::CycleState() {
    CheckState currentState = GetState();
    bool threeState = GetIsThreeState();

    CheckState newState = CheckState::Unchecked;
    if (currentState == CheckState::Unchecked) {
        newState = CheckState::Checked;
    } else if (currentState == CheckState::Checked) {
        newState = threeState ? CheckState::Indeterminate : CheckState::Unchecked;
    } else {
        newState = CheckState::Unchecked;
    }

    SetState(newState);
}

void CheckBox::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseDown(pt);
    CycleState();
}

bool CheckBox::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return false;
    }
    if (vkCode == VK_SPACE || vkCode == VK_RETURN) {
        CycleState();
        ExecuteBoundCommand();
        OnClick().Invoke(this);
        return true;
    }
    return Control::OnKeyDown(vkCode);
}

bool CheckBox::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    bool animating = base;

    CheckState state = GetState();
    float fillTarget = state == CheckState::Unchecked ? 0.0f : 1.0f;
    float checkTarget = state == CheckState::Checked ? 1.0f : 0.0f;
    float indeterminateTarget = state == CheckState::Indeterminate ? 1.0f : 0.0f;

    m_fillAnim.SetTarget(fillTarget);
    m_checkAnim.SetTarget(checkTarget);
    m_indeterminateAnim.SetTarget(indeterminateTarget);
    animating = m_fillAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.24f, 0.01f }) || animating;
    animating = m_checkAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.20f, 0.01f }) || animating;
    animating = m_indeterminateAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.20f, 0.01f }) || animating;

    if (animating) {
        MarkRenderRectDirty(m_bounds);
    }
    return animating;
}

bool CheckBox::HasSelfAnimation() const {
    float fillTarget = GetState() == CheckState::Unchecked ? 0.0f : 1.0f;
    float checkTarget = GetState() == CheckState::Checked ? 1.0f : 0.0f;
    float indeterminateTarget = GetState() == CheckState::Indeterminate ? 1.0f : 0.0f;
    return Control::HasSelfAnimation()
        || std::abs(fillTarget - m_fillAnim.Current()) > 0.01f
        || std::abs(checkTarget - m_checkAnim.Current()) > 0.01f
        || std::abs(indeterminateTarget - m_indeterminateAnim.Current()) > 0.01f;
}

} // namespace CUI
