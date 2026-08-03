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
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "Segoe UI", "Consolas", "微软雅黑", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    return metas;
}

Button::Button() {
    SetProperty("text", Value("Button"));
    SetProperty("icon", Value(""));
    SetProperty("theme.backgroundToken", Value("accentColor"));
    SetProperty("theme.hoverBackgroundToken", Value("accentColor"));
    SetProperty("theme.pressedBackgroundToken", Value("accentColor"));
    SetProperty("theme.borderToken", Value("accentColor"));
    SetProperty("theme.focusedBorderToken", Value("focusedBorder"));
    SetProperty("theme.colorToken", Value("accentForeground"));
    SetProperty("background", Value(ThemeManager::Instance().GetColor("accentColor")));
    SetProperty("hoverBackground", Value(ThemeManager::Instance().GetColor("accentColor")));
    SetProperty("pressedBackground", Value(ThemeManager::Instance().GetColor("accentColor")));
    SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor("accentColor")));
    SetProperty("focusedBorderBrush", Value(ThemeManager::Instance().GetColor("focusedBorder")));
    SetProperty("color", Value(ThemeManager::Instance().GetColor("accentForeground")));
    SetProperty("fontSize", Value(12.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("padding", Value(Thickness(8, 4, 8, 4)));
    SetProperty("cornerRadius", Value(4.0f));
    SetProperty("borderThickness", Value(0.0f));
}

Button::Button(const std::string& text) : Button() {
    SetProperty("text", Value(text));
}

Size Button::Measure(Size availableSize) {
    std::string text = GetProperty("text").AsString("Button");
    std::string icon = GetProperty("icon").AsString("");
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(12.0f);

    std::string fullText = icon.empty() ? text : (icon + " " + text);

    GraphicsContext ctx;
    Size measured = ctx.MeasureText(fullText, font, fontSize, DWRITE_FONT_WEIGHT_NORMAL);

    Thickness margin = GetProperty("margin").AsThickness(Thickness(0));
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));

    float w = measured.width + margin.left + margin.right + padding.left + padding.right;
    float h = measured.height + margin.top + margin.bottom + padding.top + padding.bottom;

    float expW = GetProperty("width").AsFloat(-1.0f);
    float expH = GetProperty("height").AsFloat(-1.0f);

    if (expW >= 0.0f) w = expW;
    if (expH >= 0.0f) h = expH;

    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void Button::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    if (!UIElement::AreAnimationsEnabled()) {
        m_rippleActive = false;
        m_rippleOpacity = 0.0f;
        return;
    }
    m_rippleCenter = pt;
    m_rippleRadius = 4.0f;
    m_rippleOpacity = 0.35f; // Soft translucent Telegram ripple
    m_rippleActive = true;
}

bool Button::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    if (!UIElement::AreAnimationsEnabled()) {
        m_rippleActive = false;
        m_rippleOpacity = 0.0f;
        return base;
    }
    if (!m_rippleActive) {
        return base;
    }

    // Telegram Style Ripple: Smooth exponential expansion from click point & gradual color fade-out
    float cornerX = (m_rippleCenter.x - m_bounds.x > m_bounds.width * 0.5f) ? m_bounds.x : (m_bounds.x + m_bounds.width);
    float cornerY = (m_rippleCenter.y - m_bounds.y > m_bounds.height * 0.5f) ? m_bounds.y : (m_bounds.y + m_bounds.height);
    float dx = m_rippleCenter.x - cornerX;
    float dy = m_rippleCenter.y - cornerY;
    float maxRadius = std::sqrt(dx * dx + dy * dy);

    m_rippleRadius += (maxRadius - m_rippleRadius) * FrameBlend(0.11f) + 72.0f * UIElement::GetAnimationDeltaSeconds();
    m_rippleOpacity *= std::pow(0.95f, UIElement::GetAnimationDeltaSeconds() * 60.0f);

    if (m_rippleOpacity <= 0.01f || m_rippleRadius >= maxRadius - 0.2f) {
        m_rippleActive = false;
        m_rippleOpacity = 0.0f;
    }

    return true;
}

bool Button::HasSelfAnimation() const {
    return Control::HasSelfAnimation() || m_rippleActive;
}

void Button::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = GetAnimatedBackground(ThemeManager::Instance().GetColor("accentColor"));
    D2D1_COLOR_F baseBorder = ResolveThemeColor("theme.borderToken", "accentColor");
    float radius = GetProperty("cornerRadius").AsFloat(4.0f);

    // 1. Draw Base Button Background
    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }

    // 2. Telegram-Style Ripple: Inside Clip, expanding soft circle with fading alpha
    if (m_rippleActive && m_rippleOpacity > 0.0f) {
        ctx.PushClip(m_bounds);
        D2D1_COLOR_F rippleColor = ThemeManager::Instance().GetColor("accentForeground");
        rippleColor.a = m_rippleOpacity;
        Rect rippleRect(
            m_rippleCenter.x - m_rippleRadius,
            m_rippleCenter.y - m_rippleRadius,
            m_rippleRadius * 2.0f,
            m_rippleRadius * 2.0f
        );
        ctx.FillRoundedRect(rippleRect, m_rippleRadius, rippleColor);
        ctx.PopClip();
    }

    // 3. Draw Clean Border (Only if borderThickness > 0 explicitly set)
    float borderThickness = GetProperty("borderThickness").AsFloat(0.0f);
    if (baseBorder.a > 0.0f && borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, baseBorder, borderThickness);
        } else {
            ctx.DrawRect(m_bounds, baseBorder, borderThickness);
        }
    }

    // 4. Render Text & Icon Content
    std::string text = GetProperty("text").AsString("");
    std::string icon = GetProperty("icon").AsString("");
    std::string fullText = icon.empty() ? text : (icon + " " + text);

    if (!fullText.empty()) {
        D2D1_COLOR_F textColor = IsEnabled()
            ? ResolveThemeColor("theme.colorToken", "accentForeground")
            : ThemeManager::Instance().GetColor("textMuted");
        std::string font = GetProperty("fontFamily").AsString("Segoe UI");
        float fontSize = GetProperty("fontSize").AsFloat(12.0f);
        Thickness padding = GetProperty("padding").AsThickness(Thickness(0));

        Rect textRect(
            m_bounds.x + padding.left,
            m_bounds.y + padding.top,
            m_bounds.width - padding.left - padding.right,
            m_bounds.height - padding.top - padding.bottom
        );

        ctx.DrawText(fullText, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

} // namespace CUI
