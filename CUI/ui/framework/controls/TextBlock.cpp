#include "TextBlock.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

std::vector<PropertyMeta> TextBlock::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "微软雅黑", "Segoe UI", "Consolas", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "fontWeight", "字体粗细 (FontWeight)", "字体文本", "enum", { "Normal", "Bold", "Light" } });
    metas.push_back({ "lineSpacing", "行间距 (LineSpacing)", "高级排版", "number" });
    metas.push_back({ "lineHeight", "固定行高 (LineHeight)", "高级排版", "number" });
    return metas;
}

TextBlock::TextBlock() {
    SetText("");
    SetColorToken(ThemeTokenId::TextSecondary);
    SetColor(ThemeManager::Instance().GetColor("textSecondary"));
    SetFontFamily("微软雅黑");
    SetFontSize(13.0f);
    SetFontWeight("Normal");
}

TextBlock::TextBlock(const std::string& text) : TextBlock() {
    SetText(text);
}

Size TextBlock::Measure(Size availableSize) {
    (void)availableSize;
    const std::string& text = GetText();
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();
    const std::string& weightStr = GetFontWeight();

    DWRITE_FONT_WEIGHT weight = (weightStr == "Bold") ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;

    // Temporary context for measuring
    GraphicsContext ctx;
    Size measured = ctx.MeasureText(text, font, fontSize, weight);

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

void TextBlock::OnRender(GraphicsContext& ctx) {
    UIElement::OnRender(ctx);

    const std::string& text = GetText();
    if (text.empty()) return;

    D2D1_COLOR_F color = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();
    const std::string& weightStr = GetFontWeight();
    const std::string& alignStr = GetTextAlign();
    const std::string& vAlignStr = GetVerticalAlign();

    DWRITE_FONT_WEIGHT weight = (weightStr == "Bold") ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;

    DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING;
    if (alignStr == "Center") align = DWRITE_TEXT_ALIGNMENT_CENTER;
    else if (alignStr == "Right") align = DWRITE_TEXT_ALIGNMENT_TRAILING;

    DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    if (vAlignStr == "Top") vAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    else if (vAlignStr == "Bottom") vAlign = DWRITE_PARAGRAPH_ALIGNMENT_FAR;

    Thickness padding = GetPadding();
    Rect textRect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        m_bounds.width - padding.left - padding.right,
        m_bounds.height - padding.top - padding.bottom
    );

    ctx.DrawText(text, textRect, color, font, fontSize, align, vAlign, weight);
}

} // namespace CUI
