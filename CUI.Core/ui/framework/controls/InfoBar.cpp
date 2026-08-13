#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "InfoBar.h"
#include "../core/Value.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1B
#endif

namespace CUI {
namespace {

constexpr float kPadX = 12.0f;
constexpr float kPadY = 10.0f;
constexpr float kBarW = 3.0f;
constexpr float kIcon = 20.0f;
constexpr float kGap = 10.0f;
constexpr float kClose = 28.0f;
constexpr float kActionH = 28.0f;
constexpr float kMinH = 48.0f;
constexpr float kRadius = 6.0f;
constexpr AnimationSpec kOpenSpec{ 0.22f, 0.01f, 0.18f };

constexpr const char* kSvgClose =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M2.2 1.1 L8 6.9 L13.8 1.1 L14.9 2.2 L9.1 8 L14.9 13.8 L13.8 14.9 "
    "L8 9.1 L2.2 14.9 L1.1 13.8 L6.9 8 L1.1 2.2 Z\"/>"
    "</svg>";

D2D1_COLOR_F WithAlpha(D2D1_COLOR_F c, float a) {
    c.a = std::clamp(a, 0.0f, 1.0f);
    return c;
}

D2D1_COLOR_F Mix(D2D1_COLOR_F a, D2D1_COLOR_F b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return D2D1::ColorF(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t);
}

D2D1_COLOR_F SuccessColor() {
    return D2D1::ColorF(0x0F7B0F);
}

D2D1_COLOR_F WarningColor() {
    return D2D1::ColorF(0x9D5D00);
}

} // namespace

InfoBar::InfoBar() {
    SetWidth(-1.0f);
    SetHeight(-1.0f);
    SetAlign(Alignment::Stretch);
    SetClipToBounds(true);
    SetCornerRadius(kRadius);
    SetBorderThickness(1.0f);
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetTitleColorToken(ThemeTokenId::TextPrimary);
    SetMessageColorToken(ThemeTokenId::TextSecondary);
    SetAccentColorToken(ThemeTokenId::AccentColor);
    EnsureChrome();
    SyncChrome();
    m_openAnim.Reset(1.0f);
}

std::vector<PropertyMeta> InfoBar::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "title", "标题 (Title)", "内容", "string" });
    metas.push_back({ "message", "正文 (Message)", "内容", "string" });
    metas.push_back({ "severity", "级别 (Severity)", "内容", "enum", { "Informational", "Success", "Warning", "Error" } });
    metas.push_back({ "isOpen", "是否打开 (IsOpen)", "内容", "bool" });
    metas.push_back({ "isClosable", "可关闭 (IsClosable)", "内容", "bool" });
    metas.push_back({ "actionText", "操作文本 (ActionText)", "内容", "string" });
    return metas;
}

namespace {
const char* SeverityName(InfoBarSeverity s) {
    switch (s) {
    case InfoBarSeverity::Success: return "Success";
    case InfoBarSeverity::Warning: return "Warning";
    case InfoBarSeverity::Error: return "Error";
    default: return "Informational";
    }
}
InfoBarSeverity ParseSeverity(const std::string& s) {
    if (s == "Success") return InfoBarSeverity::Success;
    if (s == "Warning") return InfoBarSeverity::Warning;
    if (s == "Error") return InfoBarSeverity::Error;
    return InfoBarSeverity::Informational;
}
}

Value InfoBar::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::Title:
    case PropertyId::Text:
        return Value(m_title);
    case PropertyId::Message:
    case PropertyId::Placeholder:
        return Value(m_message);
    case PropertyId::Severity: return Value(SeverityName(m_severity));
    case PropertyId::IsOpen: return Value(m_isOpen);
    case PropertyId::IsClosable: return Value(m_isClosable);
    case PropertyId::ActionText: return Value(m_actionText);
    default: return UIElement::GetProperty(id);
    }
}

bool InfoBar::HasProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::Text:
    case PropertyId::Placeholder:
        return false;
    case PropertyId::Title:
    case PropertyId::Message:
    case PropertyId::Severity:
    case PropertyId::IsOpen:
    case PropertyId::IsClosable:
    case PropertyId::ActionText:
        return true;
    default:
        return UIElement::HasProperty(id);
    }
}

void InfoBar::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::Title:
    case PropertyId::Text:
        SetTitle(val.AsString());
        return;
    case PropertyId::Message:
    case PropertyId::Placeholder:
        SetMessage(val.AsString());
        return;
    case PropertyId::Severity: SetSeverity(ParseSeverity(val.AsString())); return;
    case PropertyId::IsOpen: SetIsOpen(val.AsBool()); return;
    case PropertyId::IsClosable: SetIsClosable(val.AsBool()); return;
    case PropertyId::ActionText: SetActionText(val.AsString()); return;
    default: UIElement::SetProperty(id, val); return;
    }
}

void InfoBar::EnsureChrome() {
    if (!m_actionBtn) {
        m_actionBtn = std::make_shared<Button>("");
        m_actionBtn->SetHeight(kActionH);
        m_actionBtn->SetCornerRadius(4.0f);
        m_actionBtn->SetBorderThickness(0.0f);
        m_actionBtn->SetPadding(Thickness(12.0f, 4.0f, 12.0f, 4.0f));
        m_actionBtn->SetFontSize(12.0f);
        m_actionBtn->OnClick().Connect([this](UIElement*) { InvokeAction(); });
        AddChild(m_actionBtn);
    }
    if (!m_closeBtn) {
        m_closeBtn = std::make_shared<Button>();
        m_closeBtn->SetText("");
        m_closeBtn->SetIcon(kSvgClose);
        m_closeBtn->SetToolTip("关闭");
        m_closeBtn->SetWidth(kClose);
        m_closeBtn->SetHeight(kClose);
        m_closeBtn->SetPadding(Thickness(6.0f));
        m_closeBtn->SetCornerRadius(4.0f);
        m_closeBtn->SetBorderThickness(0.0f);
        m_closeBtn->SetBackgroundToken(ThemeTokenId::Unset);
        m_closeBtn->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        m_closeBtn->SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
        m_closeBtn->SetBackground(D2D1::ColorF(0, 0, 0, 0));
        m_closeBtn->SetColorToken(ThemeTokenId::TextSecondary);
        m_closeBtn->OnClick().Connect([this](UIElement*) { CloseFromUser(); });
        AddChild(m_closeBtn);
    }
}

void InfoBar::SyncChrome() {
    EnsureChrome();
    const bool showAction = m_isOpen && !m_actionText.empty();
    const bool showClose = m_isOpen && m_isClosable;
    m_actionBtn->SetText(m_actionText);
    m_actionBtn->SetVisibility(showAction ? Visibility::Visible : Visibility::Collapsed);
    if (m_actionCommand) {
        m_actionBtn->SetCommand(m_actionCommand);
    }
    m_closeBtn->SetVisibility(showClose ? Visibility::Visible : Visibility::Collapsed);

    const Palette pal = Colors();
    m_actionBtn->SetBackgroundToken(ThemeTokenId::Unset);
    m_actionBtn->SetHoverBackgroundToken(ThemeTokenId::Unset);
    m_actionBtn->SetPressedBackgroundToken(ThemeTokenId::Unset);
    m_actionBtn->SetBackground(pal.accent);
    m_actionBtn->SetHoverBackground(Mix(pal.accent, D2D1::ColorF(D2D1::ColorF::White), 0.12f));
    m_actionBtn->SetPressedBackground(Mix(pal.accent, D2D1::ColorF(D2D1::ColorF::Black), 0.12f));
    m_actionBtn->SetColorToken(ThemeTokenId::AccentForeground);
    m_actionBtn->SetColor(ThemeManager::Instance().GetColor(ThemeTokenId::AccentForeground));
}

void InfoBar::SetTitle(const std::string& title) {
    if (m_title == title) {
        return;
    }
    m_title = title;
    SetText(title);
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void InfoBar::SetMessage(const std::string& message) {
    if (m_message == message) {
        return;
    }
    m_message = message;
    SetPlaceholder(message);
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void InfoBar::SetSeverity(InfoBarSeverity severity) {
    if (m_severity == severity) {
        return;
    }
    m_severity = severity;
    SyncChrome();
    MarkRenderRectDirty(m_bounds);
}

void InfoBar::SetIsClosable(bool closable) {
    if (m_isClosable == closable) {
        return;
    }
    m_isClosable = closable;
    SyncChrome();
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void InfoBar::SetActionText(const std::string& text) {
    if (m_actionText == text) {
        return;
    }
    m_actionText = text;
    SyncChrome();
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void InfoBar::SetActionCommand(std::shared_ptr<Command> command) {
    m_actionCommand = std::move(command);
    SyncChrome();
}

void InfoBar::ApplyOpenState(bool open, bool animate) {
    if (open) {
        SetVisibility(Visibility::Visible);
        m_openAnim.SetTarget(1.0f);
        if (!animate || !UIElement::AreAnimationsEnabled()) {
            m_openAnim.Reset(1.0f);
        }
    } else {
        m_openAnim.SetTarget(0.0f);
        if (!animate || !UIElement::AreAnimationsEnabled()) {
            m_openAnim.Reset(0.0f);
            SetVisibility(Visibility::Collapsed);
        }
    }
    SyncChrome();
    if (animate && UIElement::AreAnimationsEnabled()) {
        RequestAnimationTicks();
    }
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds.Inflate(4.0f));
}

void InfoBar::SetIsOpen(bool open) {
    if (m_isOpen == open) {
        if (open && GetVisibility() != Visibility::Visible) {
            ApplyOpenState(true, false);
        }
        return;
    }
    m_isOpen = open;
    ApplyOpenState(open, true);
    if (!open) {
        m_onClosed.Invoke();
    }
}

void InfoBar::CloseFromUser() {
    if (!m_isOpen) {
        return;
    }
    m_isOpen = false;
    ApplyOpenState(false, true);
    m_onClosed.Invoke();
}

void InfoBar::InvokeAction() {
    m_onAction.Invoke();
}

InfoBar::Palette InfoBar::Colors() const {
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    Palette pal;
    pal.accent = tokens.accentColor;
    switch (m_severity) {
    case InfoBarSeverity::Success:
        pal.accent = SuccessColor();
        break;
    case InfoBarSeverity::Warning:
        pal.accent = WarningColor();
        break;
    case InfoBarSeverity::Error:
        pal.accent = tokens.dangerColor;
        break;
    default:
        break;
    }
    pal.fill = Mix(tokens.cardBackground, pal.accent, 0.14f);
    pal.fill.a = 1.0f;
    pal.border = Mix(tokens.cardBorder, pal.accent, 0.35f);
    pal.border.a = 1.0f;
    return pal;
}

Size InfoBar::MeasureWrapped(const std::string& text, float fontSize, float maxWidth,
                             DWRITE_FONT_WEIGHT weight) const {
    if (text.empty()) {
        return Size(0.0f, 0.0f);
    }
    GraphicsContext::TextLayoutOptions options;
    options.maxWidth = (std::max)(24.0f, maxWidth);
    options.maxHeight = 400.0f;
    options.wrapping = DWRITE_WORD_WRAPPING_WRAP;
    ComPtr<IDWriteTextLayout> layout = GraphicsContext::CreateTextLayout(
        Utf8ToUtf16(text), GetFontFamily(), fontSize, options, weight);
    if (!layout) {
        return Size(maxWidth, fontSize + 4.0f);
    }
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    return Size(
        std::ceil(metrics.widthIncludingTrailingWhitespace),
        std::ceil(metrics.height));
}

void InfoBar::DrawWrapped(
    GraphicsContext& ctx,
    const std::string& text,
    const Rect& rect,
    float fontSize,
    D2D1_COLOR_F color,
    DWRITE_FONT_WEIGHT weight) const
{
    if (text.empty() || rect.IsEmpty()) {
        return;
    }
    GraphicsContext::TextLayoutOptions options;
    options.maxWidth = (std::max)(1.0f, rect.width);
    options.maxHeight = (std::max)(1.0f, rect.height);
    options.wrapping = DWRITE_WORD_WRAPPING_WRAP;
    ComPtr<IDWriteTextLayout> layout = GraphicsContext::CreateTextLayout(
        Utf8ToUtf16(text), GetFontFamily(), fontSize, options, weight);
    if (layout) {
        ctx.DrawTextLayout(layout.Get(), rect, color);
    }
}

float InfoBar::ContentHeight(float width) const {
    const float actionW = m_actionText.empty() ? 0.0f : 88.0f;
    const float closeW = m_isClosable ? kClose : 0.0f;
    const float reserved = kPadX * 2.0f + kBarW + kGap + kIcon + kGap
        + (actionW > 0.0f ? actionW + kGap : 0.0f)
        + (closeW > 0.0f ? closeW + 4.0f : 0.0f);
    const float textW = (std::max)(48.0f, width - reserved);
    const float titleSize = GetFontSize() + 1.0f;
    const float msgSize = GetFontSize();
    const Size titleSz = MeasureWrapped(m_title, titleSize, textW, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    const Size msgSz = MeasureWrapped(m_message, msgSize, textW, DWRITE_FONT_WEIGHT_NORMAL);
    float textH = titleSz.height;
    if (msgSz.height > 0.0f) {
        textH += (textH > 0.0f ? 4.0f : 0.0f) + msgSz.height;
    }
    float chrome = kIcon;
    chrome = (std::max)(chrome, kActionH);
    chrome = (std::max)(chrome, kClose);
    chrome = (std::max)(chrome, textH);
    return (std::max)(kMinH, chrome + kPadY * 2.0f);
}

Size InfoBar::Measure(Size availableSize) {
    if (GetVisibility() == Visibility::Collapsed) {
        m_desiredSize = Size(0.0f, 0.0f);
        return m_desiredSize;
    }
    float w = GetWidth();
    if (w < 0.0f) {
        w = (availableSize.width > 0.0f && availableSize.width < 1e6f)
            ? availableSize.width
            : 480.0f;
    }
    m_lastMeasureWidth = w;
    m_contentHeight = ContentHeight(w);
    const float t = std::clamp(m_openAnim.Current(), 0.0f, 1.0f);
    m_desiredSize = Size(w, m_contentHeight * t);
    return m_desiredSize;
}

void InfoBar::Arrange(Rect finalRect) {
    const Thickness margin = GetMargin();
    Rect arranged(
        finalRect.x + margin.left,
        finalRect.y + margin.top,
        (std::max)(0.0f, finalRect.width - margin.left - margin.right),
        (std::max)(0.0f, finalRect.height - margin.top - margin.bottom));
    SetBounds(arranged);
    LayoutChrome();
    m_arrangeDirty = false;
}

void InfoBar::LayoutChrome() {
    EnsureChrome();
    const float t = std::clamp(m_openAnim.Current(), 0.0f, 1.0f);
    if (t < 0.05f || m_bounds.height < 8.0f) {
        if (m_actionBtn) {
            m_actionBtn->Arrange(Rect(m_bounds.x, m_bounds.y, 0, 0));
        }
        if (m_closeBtn) {
            m_closeBtn->Arrange(Rect(m_bounds.x, m_bounds.y, 0, 0));
        }
        return;
    }

    float right = m_bounds.x + m_bounds.width - kPadX;
    const float midY = m_bounds.y + m_bounds.height * 0.5f;
    if (m_isClosable && m_closeBtn && m_closeBtn->GetVisibility() == Visibility::Visible) {
        m_closeBtn->Arrange(Rect(right - kClose, midY - kClose * 0.5f, kClose, kClose));
        right -= kClose + 4.0f;
    } else if (m_closeBtn) {
        m_closeBtn->Arrange(Rect(m_bounds.x, m_bounds.y, 0, 0));
    }

    if (!m_actionText.empty() && m_actionBtn && m_actionBtn->GetVisibility() == Visibility::Visible) {
        Size actionSz = m_actionBtn->Measure(Size(160.0f, kActionH));
        const float aw = (std::max)(64.0f, actionSz.width);
        m_actionBtn->Arrange(Rect(right - aw, midY - kActionH * 0.5f, aw, kActionH));
    } else if (m_actionBtn) {
        m_actionBtn->Arrange(Rect(m_bounds.x, m_bounds.y, 0, 0));
    }
}

void InfoBar::DrawSeverityIcon(GraphicsContext& ctx, const Rect& slot, D2D1_COLOR_F color) const {
    const float cx = slot.x + slot.width * 0.5f;
    const float cy = slot.y + slot.height * 0.5f;
    const float r = (std::min)(slot.width, slot.height) * 0.5f;
    ctx.FillRoundedRect(slot, r, color);
    const D2D1_COLOR_F fg = ThemeManager::Instance().GetColor(ThemeTokenId::AccentForeground);

    switch (m_severity) {
    case InfoBarSeverity::Success: {
        Point check[3] = {
            Point(cx - r * 0.42f, cy + r * 0.02f),
            Point(cx - r * 0.12f, cy + r * 0.38f),
            Point(cx + r * 0.44f, cy - r * 0.32f)
        };
        ctx.DrawSmoothLine(check[0], check[1], fg, 1.8f);
        ctx.DrawSmoothLine(check[1], check[2], fg, 1.8f);
        break;
    }
    case InfoBarSeverity::Warning: {
        ctx.DrawSmoothLine(Point(cx, cy - r * 0.38f), Point(cx, cy + r * 0.08f), fg, 1.8f);
        ctx.FillRoundedRect(Rect(cx - 1.2f, cy + r * 0.28f, 2.4f, 2.4f), 1.2f, fg);
        break;
    }
    case InfoBarSeverity::Error: {
        ctx.DrawSmoothLine(
            Point(cx - r * 0.28f, cy - r * 0.28f),
            Point(cx + r * 0.28f, cy + r * 0.28f), fg, 1.7f);
        ctx.DrawSmoothLine(
            Point(cx + r * 0.28f, cy - r * 0.28f),
            Point(cx - r * 0.28f, cy + r * 0.28f), fg, 1.7f);
        break;
    }
    default: {
        ctx.FillRoundedRect(Rect(cx - 1.2f, cy - r * 0.38f, 2.4f, 2.4f), 1.2f, fg);
        ctx.DrawSmoothLine(Point(cx, cy - r * 0.08f), Point(cx, cy + r * 0.38f), fg, 1.8f);
        break;
    }
    }
}

void InfoBar::OnRender(GraphicsContext& ctx) {
    if (m_bounds.IsEmpty() || m_openAnim.Current() < 0.02f) {
        return;
    }
    const Palette pal = Colors();
    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    ctx.FillRoundedRect(m_bounds, kRadius, pal.fill);
    ctx.DrawRoundedRect(m_bounds, kRadius, pal.border, 1.0f);

    const Rect bar(
        m_bounds.x,
        m_bounds.y + 1.0f,
        kBarW,
        (std::max)(0.0f, m_bounds.height - 2.0f));
    ctx.FillRect(bar, pal.accent);

    const Rect icon(
        m_bounds.x + kPadX + kBarW,
        m_bounds.y + (m_bounds.height - kIcon) * 0.5f,
        kIcon,
        kIcon);
    DrawSeverityIcon(ctx, icon, pal.accent);

    float right = m_bounds.x + m_bounds.width - kPadX;
    if (m_isClosable) {
        right -= kClose + 4.0f;
    }
    if (!m_actionText.empty()) {
        right -= 88.0f + kGap;
    }
    const float textX = icon.x + icon.width + kGap;
    const float textW = (std::max)(8.0f, right - textX);
    const float titleSize = GetFontSize() + 1.0f;
    const float msgSize = GetFontSize();
    const Size titleSz = MeasureWrapped(m_title, titleSize, textW, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    const Size msgSz = MeasureWrapped(m_message, msgSize, textW, DWRITE_FONT_WEIGHT_NORMAL);
    float textH = titleSz.height;
    if (msgSz.height > 0.0f) {
        textH += (textH > 0.0f ? 4.0f : 0.0f) + msgSz.height;
    }
    float y = m_bounds.y + (m_bounds.height - textH) * 0.5f;
    if (!m_title.empty()) {
        DrawWrapped(
            ctx, m_title, Rect(textX, y, textW, titleSz.height),
            titleSize, tokens.textPrimary, DWRITE_FONT_WEIGHT_SEMI_BOLD);
        y += titleSz.height + 4.0f;
    }
    if (!m_message.empty()) {
        DrawWrapped(
            ctx, m_message, Rect(textX, y, textW, msgSz.height),
            msgSize, tokens.textSecondary, DWRITE_FONT_WEIGHT_NORMAL);
    }
}

bool InfoBar::OnKeyDown(int vkCode) {
    if (vkCode == VK_ESCAPE && m_isOpen && m_isClosable) {
        CloseFromUser();
        return true;
    }
    return false;
}

bool InfoBar::OnAnimationTick() {
    const float prev = m_openAnim.Current();
    const bool moving = m_openAnim.Tick(UIElement::GetAnimationDeltaSeconds(), kOpenSpec);
    if (std::abs(m_openAnim.Current() - prev) > 0.001f) {
        InvalidateMeasure();
        MarkRenderRectDirty(m_bounds.Inflate(4.0f));
    }
    if (!moving && !m_isOpen && m_openAnim.Current() <= 0.001f) {
        SetVisibility(Visibility::Collapsed);
        return false;
    }
    return moving || Control::OnAnimationTick();
}

bool InfoBar::HasSelfAnimation() const {
    return std::abs(m_openAnim.Current() - m_openAnim.Target()) > kOpenSpec.epsilon
        || Control::HasSelfAnimation();
}

void InfoBar::OnThemeChanged() {
    Control::OnThemeChanged();
    SyncChrome();
}

} // namespace CUI
