#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Button.h"
#include "../style/ThemeManager.h"
#include <windows.h>
#include <algorithm>
#include <cmath>

namespace CUI {

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
    const float fontSize = GetFontSize();

    GraphicsContext ctx;
    float contentW = 0.0f;
    float contentH = fontSize + 4.0f;
    if (!icon.empty()) {
        if (GraphicsContext::LooksLikeSvg(icon)) {
            contentW += fontSize + 2.0f;
        } else {
            const Size iconSize = ctx.MeasureText(icon, font, fontSize, ResolveFontWeight(), ResolveFontStyle(), ResolveFontStretch());
            contentW += iconSize.width;
            contentH = (std::max)(contentH, iconSize.height);
        }
    }
    if (!text.empty()) {
        const Size textSize = ctx.MeasureText(text, font, fontSize, ResolveFontWeight(), ResolveFontStyle(), ResolveFontStretch());
        if (contentW > 0.0f) {
            contentW += 6.0f;
        }
        contentW += textSize.width;
        contentH = (std::max)(contentH, textSize.height);
    }

    const Thickness margin = GetMargin();
    const Thickness padding = GetPadding();
    float w = contentW + margin.left + margin.right + padding.left + padding.right;
    float h = contentH + margin.top + margin.bottom + padding.top + padding.bottom;

    const float expW = GetWidth();
    const float expH = GetHeight();
    if (expW >= 0.0f) w = expW;
    if (expH >= 0.0f) h = expH;

    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void Button::DrawButtonFace(GraphicsContext& ctx, D2D1_COLOR_F bg, D2D1_COLOR_F border, float borderThickness) {
    const float radius = GetCornerRadius();
    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }
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
    if (text.empty() && icon.empty()) {
        return;
    }
    D2D1_COLOR_F textColor = m_hasColorValue
        ? m_colorValue
        : ResolveThemeColor(GetColorToken(), ThemeTokenId::AccentForeground);
    if (icon.empty()) {
        ctx.DrawText(
            text,
            textRect,
            textColor,
            GetFontFamily(),
            GetFontSize(),
            align,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            ResolveFontWeight(), false, ResolveFontStyle(), ResolveFontStretch(), IsUnderline(), IsStrikethrough());
        return;
    }

    const float fontSize = GetFontSize();
    const float iconSize = (std::min)(textRect.height - 2.0f, fontSize + 4.0f);
    if (text.empty()) {
        const Rect iconRect(
            textRect.x + (textRect.width - iconSize) * 0.5f,
            textRect.y + (textRect.height - iconSize) * 0.5f,
            iconSize,
            iconSize);
        ctx.DrawIcon(icon, iconRect, textColor);
        return;
    }

    const Size textSize = ctx.MeasureText(text, GetFontFamily(), fontSize, ResolveFontWeight(), ResolveFontStyle(), ResolveFontStretch());
    const float gap = 6.0f;
    const float total = iconSize + gap + textSize.width;
    float startX = textRect.x;
    if (align == DWRITE_TEXT_ALIGNMENT_CENTER) {
        startX = textRect.x + (textRect.width - total) * 0.5f;
    } else if (align == DWRITE_TEXT_ALIGNMENT_TRAILING) {
        startX = textRect.x + textRect.width - total;
    }
    const Rect iconRect(
        startX,
        textRect.y + (textRect.height - iconSize) * 0.5f,
        iconSize,
        iconSize);
    ctx.DrawIcon(icon, iconRect, textColor);
    ctx.DrawText(
        text,
        Rect(startX + iconSize + gap, textRect.y, textSize.width, textRect.height),
        textColor,
        GetFontFamily(),
        fontSize,
        DWRITE_TEXT_ALIGNMENT_LEADING,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
        ResolveFontWeight(), false, ResolveFontStyle(), ResolveFontStretch(), IsUnderline(), IsStrikethrough());
}

void Button::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    Control::OnMouseDown(pt);
}

bool Button::OnKeyDown(int vkCode) {
    if (!IsEnabled()) {
        return false;
    }
    if (vkCode == VK_SPACE || vkCode == VK_RETURN) {
        ExecuteBoundCommand();
        OnClick().Invoke(this);
        return true;
    }
    return Control::OnKeyDown(vkCode);
}

bool Button::OnAnimationTick() {
    return Control::OnAnimationTick();
}

bool Button::HasSelfAnimation() const {
    return Control::HasSelfAnimation();
}

void Button::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = GetAnimatedBackground(D2D1::ColorF(0, 0, 0, 0));
    D2D1_COLOR_F baseBorder = m_hasBorderBrushColor
        ? m_borderBrushColor
        : ((GetBorderToken() != ThemeTokenId::Unset)
            ? ResolveThemeColor(GetBorderToken(), ThemeTokenId::AccentColor)
            : D2D1::ColorF(0, 0, 0, 0));
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
