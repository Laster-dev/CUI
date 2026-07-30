#include "Button.h"

namespace CUI {

std::vector<PropertyMeta> Button::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "icon", "前置图标 (Icon)", "基本信息", "string" });
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "Segoe UI", "Consolas", "微软雅黑", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "color", "文字颜色 (Color)", "字体文本", "color" });
    return metas;
}

Button::Button() {
    SetProperty("text", Value("Button"));
    SetProperty("icon", Value(""));
    SetProperty("background", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f))); // VS Code primary blue
    SetProperty("hoverBackground", Value(D2D1::ColorF(0x00 / 255.0f, 0x98 / 255.0f, 0xFF / 255.0f, 1.0f)));
    SetProperty("pressedBackground", Value(D2D1::ColorF(0x00 / 255.0f, 0x5A / 255.0f, 0x9E / 255.0f, 1.0f)));
    SetProperty("color", Value(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)));
    SetProperty("fontSize", Value(12.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("padding", Value(Thickness(8, 4, 8, 4)));
    SetProperty("cornerRadius", Value(2.0f));
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

void Button::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));
    if (!IsEnabled()) {
        bg = GetProperty("disabledBackground").AsColor(D2D1::ColorF(0x3A / 255.0f, 0x3A / 255.0f, 0x3A / 255.0f, 1.0f));
    } else if (m_isPressed) {
        bg = GetProperty("pressedBackground").AsColor(bg);
    } else if (m_isHovered) {
        bg = GetProperty("hoverBackground").AsColor(bg);
    }

    float radius = GetProperty("cornerRadius").AsFloat(2.0f);
    if (radius > 0.0f) {
        ctx.FillRoundedRect(m_bounds, radius, bg);
    } else {
        ctx.FillRect(m_bounds, bg);
    }

    D2D1_COLOR_F borderBrush = GetProperty("borderBrush").AsColor(D2D1::ColorF(0, 0, 0, 0));
    float borderThickness = GetProperty("borderThickness").AsFloat(0.0f);
    if (borderBrush.a > 0.0f && borderThickness > 0.0f) {
        if (radius > 0.0f) {
            ctx.DrawRoundedRect(m_bounds, radius, borderBrush, borderThickness);
        } else {
            ctx.DrawRect(m_bounds, borderBrush, borderThickness);
        }
    }

    std::string text = GetProperty("text").AsString("");
    std::string icon = GetProperty("icon").AsString("");
    std::string fullText = icon.empty() ? text : (icon + " " + text);

    if (!fullText.empty()) {
        D2D1_COLOR_F textColor = IsEnabled()
        ? GetProperty("color").AsColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f))
        : D2D1::ColorF(0x7A / 255.0f, 0x7A / 255.0f, 0x7A / 255.0f, 1.0f);
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
