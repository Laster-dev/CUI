#include "TextBlock.h"
#include "../core/CUIDsl.h"
#include "../style/ThemeManager.h"
#include "../core/Value.h"
#include <algorithm>
#include <cmath>

namespace CUI {

Value TextBlock::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::LineSpacing: return Value(m_lineSpacing);
    case PropertyId::LineHeight: return Value(m_lineHeight);
    default: return UIElement::GetProperty(id);
    }
}

bool TextBlock::HasProperty(PropertyId id) const {
    return id == PropertyId::LineSpacing || id == PropertyId::LineHeight || UIElement::HasProperty(id);
}

void TextBlock::ApplyProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::LineSpacing: ApplyLineSpacing(val.AsFloat()); return;
    case PropertyId::LineHeight: ApplyLineHeight(val.AsFloat()); return;
    default: UIElement::ApplyProperty(id, val); return;
    }
}

TextBlock::TextBlock() {
    this->ApplyText("");
    this->ApplyColorToken(ThemeTokenId::TextPrimary);
    this->ApplyFontFamily("微软雅黑");
    this->ApplyFontSize(12.0f);
    this->ApplyFontWeight(CUI::FontWeight::Normal);
}

TextBlock::TextBlock(const std::string& text) : TextBlock() {
    this->ApplyText(text);
}

Size TextBlock::Measure(Size availableSize) {
    (void)availableSize;
    const std::string& text = GetText();
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();

    CUI::FontWeight weight = GetFontWeight();

    GraphicsContext ctx;
    Size measured;
    const bool customLead = m_lineHeight > 0.0f || std::abs(m_lineSpacing - 1.0f) > 0.001f;
    if (customLead) {
        TextLayoutOptions opt;
        opt.lineSpacing = m_lineSpacing;
        opt.lineHeight = m_lineHeight;
        TextLayout layout = TextLayout::Create(text, font, fontSize, opt, weight, GetFontStyle(), GetFontStretch());
        if (layout) {
            measured = layout.GetSize();
        } else {
            measured = ctx.MeasureText(text, font, fontSize, weight, GetFontStyle(), GetFontStretch());
        }
    } else {
        measured = ctx.MeasureText(text, font, fontSize, weight, GetFontStyle(), GetFontStretch());
    }

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

    Color color = m_hasColorValue
        ? m_colorValue
        : ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();
    CUI::FontWeight weight = GetFontWeight();
    TextAlignment align = GetTextAlign();
    TextVerticalAlignment vAlign = GetVerticalAlign();

    Thickness padding = GetPadding();
    Rect textRect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        m_bounds.width - padding.left - padding.right,
        m_bounds.height - padding.top - padding.bottom
    );

    const bool customLead = m_lineHeight > 0.0f || std::abs(m_lineSpacing - 1.0f) > 0.001f;
    if (customLead) {
        TextLayoutOptions opt;
        opt.maxWidth = textRect.width;
        opt.maxHeight = textRect.height;
        opt.wrapping = TextWrapping::Wrap;
        opt.paragraphAlignment = vAlign;
        opt.lineSpacing = m_lineSpacing;
        opt.lineHeight = m_lineHeight;
        TextLayout layout = TextLayout::Create(text, font, fontSize, opt, weight, GetFontStyle(), GetFontStretch());
        if (layout) {
            ctx.DrawTextLayout(layout, textRect, color);
            return;
        }
    }
    ctx.DrawText(text, textRect, color, font, fontSize, align, vAlign, weight, false, GetFontStyle(), GetFontStretch(), IsUnderline(), IsStrikethrough());
}

} // namespace CUI
