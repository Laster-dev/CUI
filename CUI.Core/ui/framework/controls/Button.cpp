#include "Button.h"
#include "../style/ThemeManager.h"
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

std::vector<PropertyMeta> Button::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "icon", "前置图标 (Icon)", "基本信息", "string" });
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "微软雅黑", "Segoe UI", "Consolas", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    return metas;
}

Button::Button() {
    SetText("Button");
    SetBackgroundToken(ThemeTokenId::AccentColor);
    SetHoverBackgroundToken(ThemeTokenId::AccentColor);
    SetPressedBackgroundToken(ThemeTokenId::AccentColor);
    SetBorderToken(ThemeTokenId::AccentColor);
    SetFocusedBorderToken(ThemeTokenId::FocusedBorder);
    SetColorToken(ThemeTokenId::AccentForeground);
    SetBackground(ThemeManager::Instance().GetColor("accentColor"));
    SetHoverBackground(ThemeManager::Instance().GetColor("accentColor"));
    SetPressedBackground(ThemeManager::Instance().GetColor("accentColor"));
    SetBorderBrush(ThemeManager::Instance().GetColor("accentColor"));
    SetColor(ThemeManager::Instance().GetColor("accentForeground"));
    SetFontSize(12.0f);
    SetFontFamily("微软雅黑");
    SetPadding(Thickness(8, 4, 8, 4));
    SetCornerRadius(4.0f);
    SetBorderThickness(0.0f);
}

Button::Button(const std::string& text) : Button() {
    SetText(text);
}

Size Button::Measure(Size availableSize) {
    (void)availableSize;
    const std::string& text = GetText();
    const std::string& icon = GetIcon();
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();

    std::string fullText = icon.empty() ? text : (icon + " " + text);

    GraphicsContext ctx;
    Size measured = ctx.MeasureText(fullText, font, fontSize, DWRITE_FONT_WEIGHT_NORMAL);

    Thickness margin = GetMargin();
    Thickness padding = GetPadding();

    float w = measured.width + margin.left + margin.right + padding.left + padding.right;
    float h = measured.height + margin.top + margin.bottom + padding.top + padding.bottom;

    float expW = GetWidth();
    float expH = GetHeight();

    if (expW >= 0.0f) w = expW;
    if (expH >= 0.0f) h = expH;

    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void Button::BeginRipple(Point pt) {
    if (!UIElement::AreAnimationsEnabled()) {
        m_rippleActive = false;
        m_rippleOpacity = 0.0f;
        return;
    }
    m_rippleCenter = pt;
    m_rippleRadius = 4.0f;
    m_rippleOpacity = 0.35f;
    m_rippleActive = true;
    RequestAnimationTicks();
    MarkRenderRectDirty(m_bounds);
}

bool Button::TickRipple() {
    if (!UIElement::AreAnimationsEnabled()) {
        m_rippleActive = false;
        m_rippleOpacity = 0.0f;
        return false;
    }
    if (!m_rippleActive) {
        return false;
    }

    float cornerX = (m_rippleCenter.x - m_bounds.x > m_bounds.width * 0.5f) ? m_bounds.x : (m_bounds.x + m_bounds.width);
    float cornerY = (m_rippleCenter.y - m_bounds.y > m_bounds.height * 0.5f) ? m_bounds.y : (m_bounds.y + m_bounds.height);
    float dx = m_rippleCenter.x - cornerX;
    float dy = m_rippleCenter.y - cornerY;
    float maxRadius = std::sqrt(dx * dx + dy * dy);

    m_rippleRadius += (maxRadius - m_rippleRadius) * FrameBlend(0.073f) + 37.0f * UIElement::GetAnimationDeltaSeconds();
    if (m_rippleRadius > maxRadius) {
        m_rippleRadius = maxRadius;
    }
    m_rippleOpacity *= std::pow(0.958f, UIElement::GetAnimationDeltaSeconds() * 60.0f);

    if (m_rippleOpacity <= 0.02f) {
        m_rippleActive = false;
        m_rippleOpacity = 0.0f;
    }

    MarkRenderRectDirty(m_bounds);
    return true;
}

void Button::DrawRipple(GraphicsContext& ctx) {
    if (!m_rippleActive || m_rippleOpacity <= 0.0f) {
        return;
    }
    const float radius = GetCornerRadius();
    ctx.PushRoundedClip(m_bounds, radius);
    D2D1_COLOR_F rippleColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary);
    rippleColor.a = m_rippleOpacity;
    Rect rippleRect(
        m_rippleCenter.x - m_rippleRadius,
        m_rippleCenter.y - m_rippleRadius,
        m_rippleRadius * 2.0f,
        m_rippleRadius * 2.0f);
    ctx.FillRoundedRect(rippleRect, m_rippleRadius, rippleColor);
    ctx.PopClip();
}

void Button::DrawButtonFace(GraphicsContext& ctx, D2D1_COLOR_F bg, D2D1_COLOR_F border, float borderThickness) {
    const float radius = GetCornerRadius();
    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }
    DrawRipple(ctx);
    if (border.a > 0.0f && borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, border, borderThickness);
        } else {
            ctx.DrawRect(m_bounds, border, borderThickness);
        }
    }
}

void Button::DrawButtonLabel(GraphicsContext& ctx, const Rect& textRect, DWRITE_TEXT_ALIGNMENT align) {
    const std::string& text = GetText();
    const std::string& icon = GetIcon();
    const std::string fullText = icon.empty() ? text : (icon + " " + text);
    if (fullText.empty()) {
        return;
    }
    D2D1_COLOR_F textColor = IsEnabled()
        ? ResolveThemeColor(GetColorToken(), ThemeTokenId::AccentForeground)
        : ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextMuted);
    ctx.DrawText(
        fullText,
        textRect,
        textColor,
        GetFontFamily(),
        GetFontSize(),
        align,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void Button::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseDown(pt);
    BeginRipple(pt);
}

bool Button::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    return TickRipple() || base;
}

bool Button::HasSelfAnimation() const {
    return Control::HasSelfAnimation() || m_rippleActive;
}

void Button::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = GetAnimatedBackground(ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentColor));
    D2D1_COLOR_F baseBorder = ResolveThemeColor(GetBorderToken(), ThemeTokenId::AccentColor);
    DrawButtonFace(ctx, bg, baseBorder, GetBorderThickness());

    Thickness padding = GetPadding();
    Rect textRect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        m_bounds.width - padding.left - padding.right,
        m_bounds.height - padding.top - padding.bottom);
    DrawButtonLabel(ctx, textRect, DWRITE_TEXT_ALIGNMENT_CENTER);
}

} // namespace CUI
