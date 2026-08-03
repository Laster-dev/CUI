#include "TextBlock.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

std::vector<PropertyMeta> TextBlock::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "Segoe UI", "Consolas", "微软雅黑", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "fontWeight", "字体粗细 (FontWeight)", "字体文本", "enum", { "Normal", "Bold", "Light" } });
    metas.push_back({ "lineSpacing", "行间距 (LineSpacing)", "高级排版", "number" });
    metas.push_back({ "lineHeight", "固定行高 (LineHeight)", "高级排版", "number" });
    return metas;
}

TextBlock::TextBlock() {
    SetProperty("text", Value(""));
    SetProperty("theme.colorToken", Value("textSecondary"));
    SetProperty("color", Value(ThemeManager::Instance().GetColor("textSecondary")));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("fontSize", Value(13.0f));
    SetProperty("fontWeight", Value("Normal"));
    SetProperty("textAlign", Value("Left"));
    SetProperty("verticalAlign", Value("Center"));
}

TextBlock::TextBlock(const std::string& text) : TextBlock() {
    SetProperty("text", Value(text));
}

Size TextBlock::Measure(Size availableSize) {
    std::string text = GetProperty("text").AsString("");
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);
    std::string weightStr = GetProperty("fontWeight").AsString("Normal");

    DWRITE_FONT_WEIGHT weight = (weightStr == "Bold") ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;

    // Temporary context for measuring
    GraphicsContext ctx;
    Size measured = ctx.MeasureText(text, font, fontSize, weight);

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

void TextBlock::OnRender(GraphicsContext& ctx) {
    UIElement::OnRender(ctx);

    std::string text = GetProperty("text").AsString("");
    if (text.empty()) return;

    D2D1_COLOR_F color = ResolveThemeColor("theme.colorToken", "textSecondary");
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);
    std::string weightStr = GetProperty("fontWeight").AsString("Normal");
    std::string alignStr = GetProperty("textAlign").AsString("Left");
    std::string vAlignStr = GetProperty("verticalAlign").AsString("Center");

    DWRITE_FONT_WEIGHT weight = (weightStr == "Bold") ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;

    DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING;
    if (alignStr == "Center") align = DWRITE_TEXT_ALIGNMENT_CENTER;
    else if (alignStr == "Right") align = DWRITE_TEXT_ALIGNMENT_TRAILING;

    DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    if (vAlignStr == "Top") vAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    else if (vAlignStr == "Bottom") vAlign = DWRITE_PARAGRAPH_ALIGNMENT_FAR;

    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));
    Rect textRect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        m_bounds.width - padding.left - padding.right,
        m_bounds.height - padding.top - padding.bottom
    );

    ctx.DrawText(text, textRect, color, font, fontSize, align, vAlign, weight);
}

} // namespace CUI
