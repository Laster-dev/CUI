#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "NumberBox.h"
#include "../style/ThemeManager.h"
#include "../core/Value.h"
#include "../input/RoutedEvent.h"
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

void NumberBox::Field::OnRoutedEvent(RoutedEventArgs& args) {
    if (args.handled) {
        return;
    }
    if (args.type == RoutedEventType::KeyDown && host && host->HandleFieldKey(args.keyCode)) {
        args.handled = true;
        return;
    }
    TextBox::OnRoutedEvent(args);
}

void NumberBox::Field::OnCharInput(wchar_t ch) {
    if (ch >= 32 && !IsNumericEditChar(ch)) {
        return;
    }
    TextBox::OnCharInput(ch);
}

void NumberBox::Field::OnMouseWheel(float delta) {
    if (host) {
        host->StepBy(delta > 0.0f ? 1.0f : -1.0f);
        return;
    }
    TextBox::OnMouseWheel(delta);
}

void NumberBox::Field::OnBlur() {
    if (host) {
        host->CommitEdit();
    }
    TextBox::OnBlur();
}

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

    m_field = std::make_shared<Field>();
    m_field->host = this;
    m_field->SetFontFamily(GetFontFamily());
    m_field->SetFontSize(GetFontSize());
    m_field->SetPadding(Thickness(2.0f, 0.0f, 2.0f, 0.0f));
    m_field->SetBorderThickness(0.0f);
    m_field->SetBackground(D2D1::ColorF(0, 0, 0, 0));
    m_field->SetHoverBackground(D2D1::ColorF(0, 0, 0, 0));
    m_field->SetBackgroundToken(ThemeTokenId::Unset);
    m_field->SetHoverBackgroundToken(ThemeTokenId::Unset);
    m_field->SetUnderlineColorToken(ThemeTokenId::Unset);
    m_field->SetActiveUnderlineColorToken(ThemeTokenId::Unset);
    m_field->SetColorToken(ThemeTokenId::TextPrimary);
    m_field->SetAcceptsReturn(false);
    m_field->OnTextChanged().Connect([this](TextBox*, const std::string&) {
        OnFieldTextChanged();
    });
    AddChild(m_field);
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
    m_arrangeDirty = false;
    LayoutField();
}

void NumberBox::LayoutField() {
    if (!m_field) {
        return;
    }
    m_field->Arrange(TextRect());
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
    if (!m_field) {
        return;
    }
    m_syncingText = true;
    m_field->SetText(FormatValue(m_value));
    m_syncingText = false;
}

void NumberBox::SetValue(float val) {
    const float clamped = std::clamp(val, m_minimum, m_maximum);
    if (std::abs(clamped - m_value) <= 0.0001f) {
        SyncTextFromValue();
        return;
    }
    m_value = clamped;
    NotifyFieldChanged(PropertyId::ControlValue, Value(clamped));
    SyncTextFromValue();
    m_onValueChangedEvent.Invoke(this, clamped);
    MarkRenderContentDirty();
}

void NumberBox::OnFieldTextChanged() {
    if (m_syncingText || !m_field) {
        return;
    }
    const std::string& text = m_field->GetText();
    if (text.empty() || text == "-" || text == "." || text == "-.") {
        return;
    }
    try {
        const float parsed = std::stof(text);
        const float clamped = std::clamp(parsed, m_minimum, m_maximum);
        if (std::abs(clamped - m_value) <= 0.0001f) {
            return;
        }
        m_value = clamped;
        NotifyFieldChanged(PropertyId::ControlValue, Value(clamped));
        m_onValueChangedEvent.Invoke(this, clamped);
    } catch (...) {
    }
}

void NumberBox::CommitEdit() {
    if (!m_field) {
        return;
    }
    float parsed = m_value;
    const std::string& text = m_field->GetText();
    try {
        if (!text.empty() && text != "-" && text != "." && text != "-.") {
            parsed = std::stof(text);
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

bool NumberBox::HandleFieldKey(int vkCode) {
    if (vkCode == VK_UP) {
        StepBy(1.0f);
        return true;
    }
    if (vkCode == VK_DOWN) {
        StepBy(-1.0f);
        return true;
    }
    if (vkCode == VK_RETURN) {
        CommitEdit();
        return true;
    }
    if (vkCode == VK_ESCAPE) {
        SyncTextFromValue();
        return true;
    }
    return false;
}

void NumberBox::OnRender(GraphicsContext& ctx) {
    const float radius = GetCornerRadius();
    D2D1_COLOR_F bg = GetAnimatedBackground(ThemeManager::Instance().GetFlatColor(ThemeTokenId::InputBackground));
    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }

    const bool fieldFocused = m_field && m_field->IsFocused();
    D2D1_COLOR_F border = (m_isFocused || fieldFocused)
        ? ResolveThemeColor(GetFocusedBorderToken(), ThemeTokenId::FocusedBorder)
        : ResolveThemeColor(GetBorderToken(), ThemeTokenId::InputBorder);
    const float borderW = (m_isFocused || fieldFocused) ? 1.5f : GetBorderThickness();
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

bool NumberBox::OnKeyDown(int vkCode) {
    if (HandleFieldKey(vkCode)) {
        return true;
    }
    return Control::OnKeyDown(vkCode);
}

bool NumberBox::OnAnimationTick() {
    bool any = Control::OnAnimationTick();
    const float dt = GetAnimationDeltaSeconds();
    any = m_hotUp.Tick(dt, SpinnerHoverSpec()) || any;
    any = m_hotDown.Tick(dt, SpinnerHoverSpec()) || any;

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
        || m_pressed == HitPart::Up || m_pressed == HitPart::Down
        || std::abs(m_hotUp.Current() - m_hotUp.Target()) > 0.01f
        || std::abs(m_hotDown.Current() - m_hotDown.Target()) > 0.01f;
}

} // namespace CUI
