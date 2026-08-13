#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "NumberBox.h"
#include "../style/ThemeManager.h"
#include "../core/Value.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <windows.h>

namespace CUI {

namespace {
AnimationSpec SpinnerHoverSpec() {
    AnimationSpec s;
    s.responseAt60Hz = 0.22f;
    s.epsilon = 0.01f;
    return s;
}

bool IsNumericEditChar(wchar_t ch) {
    return (ch >= L'0' && ch <= L'9') || ch == L'.' || ch == L'-';
}
} // namespace

NumberBox::NumberBox() {
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    SetWidth(120.0f);
    SetHeight(28.0f);
    SetPadding(Thickness(8.0f, 4.0f, 4.0f, 4.0f));
    SetCornerRadius(3.0f);
    SetFontSize(13.0f);
    SetFontFamily("Segoe UI");

    SetBackgroundToken(ThemeTokenId::InputBackground);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetBorderToken(ThemeTokenId::InputBorder);
    SetFocusedBorderToken(ThemeTokenId::FocusedBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetBackground(tokens.inputBackground);
    SetBorderBrush(tokens.inputBorder);
    SetColor(tokens.textPrimary);
    SetBorderThickness(1.0f);
    SyncTextFromValue();
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
    (void)availableSize;
    float expW = GetWidth(); if (expW < 0) expW = 120.0f;
    float expH = GetHeight(); if (expH < 0) expH = 28.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void NumberBox::Arrange(Rect finalRect) {
    m_bounds = finalRect;
}

Rect NumberBox::SpinnerCol() const {
    const float border = GetBorderThickness();
    return Rect(
        m_bounds.x + m_bounds.width - kSpinnerW - border,
        m_bounds.y + border,
        kSpinnerW,
        (std::max)(0.0f, m_bounds.height - border * 2.0f));
}

Rect NumberBox::UpBtn() const {
    const Rect col = SpinnerCol();
    return Rect(col.x, col.y, col.width, col.height * 0.5f);
}

Rect NumberBox::DownBtn() const {
    const Rect col = SpinnerCol();
    return Rect(col.x, col.y + col.height * 0.5f, col.width, col.height * 0.5f);
}

Rect NumberBox::TextRect() const {
    const Thickness pad = GetPadding();
    const float border = GetBorderThickness();
    return Rect(
        m_bounds.x + border + pad.left,
        m_bounds.y + border,
        (std::max)(0.0f, m_bounds.width - border * 2.0f - kSpinnerW - pad.left - 2.0f),
        (std::max)(0.0f, m_bounds.height - border * 2.0f));
}

NumberBox::HitPart NumberBox::HitTestPart(Point pt) const {
    if (UpBtn().Contains(pt.x, pt.y)) {
        return HitPart::Up;
    }
    if (DownBtn().Contains(pt.x, pt.y)) {
        return HitPart::Down;
    }
    if (m_bounds.Contains(pt.x, pt.y)) {
        return HitPart::Text;
    }
    return HitPart::None;
}

HCURSOR NumberBox::GetCursor() const {
    if (!IsEnabled()) {
        return nullptr;
    }
    if (m_hover == HitPart::Up || m_hover == HitPart::Down) {
        return LoadCursor(nullptr, IDC_HAND);
    }
    if (m_hover == HitPart::Text || m_isFocused) {
        return LoadCursor(nullptr, IDC_IBEAM);
    }
    return nullptr;
}

std::string NumberBox::FormatValue(float val) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << val;
    std::string s = ss.str();
    if (s.size() > 2 && s.substr(s.size() - 2) == ".0") {
        s = s.substr(0, s.size() - 2);
    }
    return s;
}

void NumberBox::SyncTextFromValue() {
    m_editText = FormatValue(m_value);
    m_caret = static_cast<int>(m_editText.size());
}

void NumberBox::SetValue(float val) {
    const float clamped = std::clamp(val, m_minimum, m_maximum);
    if (std::abs(clamped - m_value) <= 0.0001f) {
        return;
    }
    m_value = clamped;
    NotifyFieldChanged(PropertyId::ControlValue, Value(clamped));
    SyncTextFromValue();
    m_onValueChangedEvent.Invoke(this, clamped);
    MarkRenderContentDirty();
}

void NumberBox::CommitEdit() {
    float parsed = m_value;
    try {
        if (!m_editText.empty() && m_editText != "-" && m_editText != "." && m_editText != "-.") {
            parsed = std::stof(m_editText);
        }
    } catch (...) {
        parsed = m_value;
    }
    SetValue(parsed);
}

void NumberBox::StepBy(float dir) {
    CommitEdit();
    SetValue(m_value + dir * m_step);
}

void NumberBox::SetCaret(int pos) {
    m_caret = std::clamp(pos, 0, static_cast<int>(m_editText.size()));
    m_caretVisible = true;
    m_caretBlink = 0.0f;
    MarkRenderContentDirty();
}

void NumberBox::InsertChar(char ch) {
    if (ch == '-' && (m_caret != 0 || (!m_editText.empty() && m_editText[0] == '-'))) {
        return;
    }
    if (ch == '.' && m_editText.find('.') != std::string::npos) {
        return;
    }
    m_editText.insert(m_editText.begin() + m_caret, ch);
    SetCaret(m_caret + 1);
}

void NumberBox::DeleteBackward() {
    if (m_caret <= 0 || m_editText.empty()) {
        return;
    }
    m_editText.erase(m_editText.begin() + (m_caret - 1));
    SetCaret(m_caret - 1);
}

void NumberBox::DeleteForward() {
    if (m_caret >= static_cast<int>(m_editText.size())) {
        return;
    }
    m_editText.erase(m_editText.begin() + m_caret);
    SetCaret(m_caret);
}

void NumberBox::OnRender(GraphicsContext& ctx) {
    const float radius = GetCornerRadius();
    D2D1_COLOR_F bg = GetAnimatedBackground(ThemeManager::Instance().GetFlatColor(ThemeTokenId::InputBackground));
    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }

    D2D1_COLOR_F border = m_isFocused
        ? ResolveThemeColor(GetFocusedBorderToken(), ThemeTokenId::FocusedBorder)
        : ResolveThemeColor(GetBorderToken(), ThemeTokenId::InputBorder);
    const float borderW = m_isFocused ? 1.5f : GetBorderThickness();
    if (borderW > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, border, borderW);
        } else {
            ctx.DrawRect(m_bounds, border, borderW);
        }
    }

    const auto& tokens = ThemeManager::Instance().GetTokens();
    const Rect up = UpBtn();
    const Rect down = DownBtn();
    const float upT = m_hotUp.Current();
    const float downT = m_hotDown.Current();
    if (upT > 0.01f) {
        D2D1_COLOR_F fill = tokens.textPrimary;
        fill.a = 0.08f + 0.10f * upT;
        ctx.FillRect(up, fill);
    }
    if (downT > 0.01f) {
        D2D1_COLOR_F fill = tokens.textPrimary;
        fill.a = 0.08f + 0.10f * downT;
        ctx.FillRect(down, fill);
    }

    const Rect col = SpinnerCol();
    ctx.DrawLine(
        Point(col.x, m_bounds.y + 4.0f),
        Point(col.x, m_bounds.y + m_bounds.height - 4.0f),
        tokens.cardBorder,
        1.0f);
    ctx.DrawLine(
        Point(col.x + 3.0f, col.y + col.height * 0.5f),
        Point(col.x + col.width - 3.0f, col.y + col.height * 0.5f),
        tokens.cardBorder,
        1.0f);

    D2D1_COLOR_F chevron = tokens.textSecondary;
    ctx.DrawChevron(up, chevron, GraphicsContext::ChevronDirection::Up, 1.3f);
    ctx.DrawChevron(down, chevron, GraphicsContext::ChevronDirection::Down, 1.3f);

    const Rect textRect = TextRect();
    ctx.DrawText(
        m_editText,
        textRect,
        ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary),
        GetFontFamily(),
        GetFontSize(),
        DWRITE_TEXT_ALIGNMENT_LEADING,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    if (m_isFocused && m_caretVisible && IsEnabled()) {
        GraphicsContext probe;
        const std::string prefix = m_editText.substr(0, static_cast<size_t>(m_caret));
        const float prefixW = probe.MeasureText(prefix, GetFontFamily(), GetFontSize()).width;
        const float caretX = textRect.x + prefixW;
        const float caretH = (std::max)(12.0f, textRect.height - 8.0f);
        const float caretY = textRect.y + (textRect.height - caretH) * 0.5f;
        ctx.FillRect(
            Rect(caretX, caretY, 1.0f, caretH),
            ResolveThemeColor(GetCaretColorToken(), ThemeTokenId::AccentColor));
    }
}

void NumberBox::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    m_pressed = HitTestPart(pt);
    m_holdAcc = 0.0f;
    m_holdRepeat = false;
    if (m_pressed == HitPart::Up) {
        StepBy(1.0f);
        RequestAnimationTicks();
    } else if (m_pressed == HitPart::Down) {
        StepBy(-1.0f);
        RequestAnimationTicks();
    } else if (m_pressed == HitPart::Text) {
        GraphicsContext probe;
        const Rect textRect = TextRect();
        const float rel = pt.x - textRect.x;
        int pos = static_cast<int>(m_editText.size());
        for (int i = 0; i <= static_cast<int>(m_editText.size()); ++i) {
            const float w = probe.MeasureText(m_editText.substr(0, static_cast<size_t>(i)), GetFontFamily(), GetFontSize()).width;
            if (rel <= w + 3.0f) {
                pos = i;
                break;
            }
        }
        SetCaret(pos);
    }
}

void NumberBox::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_pressed = HitPart::None;
    m_holdRepeat = false;
    m_holdAcc = 0.0f;
}

void NumberBox::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    const HitPart next = HitTestPart(pt);
    if (next != m_hover) {
        m_hover = next;
        MarkRenderContentDirty();
    }
    m_hotUp.SetTarget(m_hover == HitPart::Up ? 1.0f : 0.0f);
    m_hotDown.SetTarget(m_hover == HitPart::Down ? 1.0f : 0.0f);
    RequestAnimationTicks();
}

void NumberBox::OnMouseLeave() {
    Control::OnMouseLeave();
    m_hover = HitPart::None;
    m_hotUp.SetTarget(0.0f);
    m_hotDown.SetTarget(0.0f);
    RequestAnimationTicks();
    MarkRenderContentDirty();
}

void NumberBox::OnMouseWheel(float delta) {
    if (!IsEnabled()) {
        return;
    }
    StepBy(delta > 0.0f ? 1.0f : -1.0f);
}

void NumberBox::OnKeyDown(int vkCode) {
    if (vkCode == VK_UP) {
        StepBy(1.0f);
        return;
    }
    if (vkCode == VK_DOWN) {
        StepBy(-1.0f);
        return;
    }
    if (vkCode == VK_LEFT) {
        SetCaret(m_caret - 1);
        return;
    }
    if (vkCode == VK_RIGHT) {
        SetCaret(m_caret + 1);
        return;
    }
    if (vkCode == VK_HOME) {
        SetCaret(0);
        return;
    }
    if (vkCode == VK_END) {
        SetCaret(static_cast<int>(m_editText.size()));
        return;
    }
    if (vkCode == VK_BACK) {
        DeleteBackward();
        return;
    }
    if (vkCode == VK_DELETE) {
        DeleteForward();
        return;
    }
    if (vkCode == VK_RETURN) {
        CommitEdit();
        return;
    }
    if (vkCode == VK_ESCAPE) {
        SyncTextFromValue();
        MarkRenderContentDirty();
        return;
    }
    Control::OnKeyDown(vkCode);
}

void NumberBox::OnCharInput(wchar_t ch) {
    if (!IsEnabled() || ch < 32) {
        return;
    }
    if (!IsNumericEditChar(ch)) {
        return;
    }
    InsertChar(static_cast<char>(ch));
}

void NumberBox::OnFocus() {
    Control::OnFocus();
    m_caretVisible = true;
    m_caretBlink = 0.0f;
    RequestAnimationTicks();
    MarkRenderContentDirty();
}

void NumberBox::OnBlur() {
    CommitEdit();
    Control::OnBlur();
    m_caretVisible = false;
    MarkRenderContentDirty();
}

bool NumberBox::OnAnimationTick() {
    bool any = Control::OnAnimationTick();
    const float dt = GetAnimationDeltaSeconds();
    any = m_hotUp.Tick(dt, SpinnerHoverSpec()) || any;
    any = m_hotDown.Tick(dt, SpinnerHoverSpec()) || any;

    if (m_isFocused) {
        m_caretBlink += dt;
        if (m_caretBlink >= 0.53f) {
            m_caretBlink = 0.0f;
            m_caretVisible = !m_caretVisible;
            any = true;
        }
    }

    if (m_pressed == HitPart::Up || m_pressed == HitPart::Down) {
        m_holdAcc += dt;
        if (!m_holdRepeat && m_holdAcc >= 0.40f) {
            m_holdRepeat = true;
            m_holdAcc = 0.0f;
        }
        if (m_holdRepeat && m_holdAcc >= 0.05f) {
            m_holdAcc = 0.0f;
            StepBy(m_pressed == HitPart::Up ? 1.0f : -1.0f);
            any = true;
        }
        any = true;
    }

    if (any) {
        MarkRenderContentDirty();
    }
    return any;
}

bool NumberBox::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || m_isFocused
        || m_pressed == HitPart::Up || m_pressed == HitPart::Down
        || std::abs(m_hotUp.Current() - m_hotUp.Target()) > 0.01f
        || std::abs(m_hotDown.Current() - m_hotDown.Target()) > 0.01f;
}

} // namespace CUI
