#include "TextBlock.h"
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

void TextBlock::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::LineSpacing: SetLineSpacing(val.AsFloat()); return;
    case PropertyId::LineHeight: SetLineHeight(val.AsFloat()); return;
    default: UIElement::SetProperty(id, val); return;
    }
}

TextBlock::TextBlock() {
    SetText("");
    SetColorToken(ThemeTokenId::TextSecondary);
    SetColor(ThemeManager::Instance().GetColor("textSecondary"));
    SetFontFamily("微软雅黑");
    SetFontSize(12.0f);
    SetFontWeight(CUI::FontWeight::Normal);
}

TextBlock::TextBlock(const std::string& text) : TextBlock() {
    SetText(text);
}

Size TextBlock::Measure(Size availableSize) {
    (void)availableSize;
    const std::string& text = GetText();
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();

    DWRITE_FONT_WEIGHT weight = ResolveFontWeight();

    GraphicsContext ctx;
    Size measured;
    const bool customLead = m_lineHeight > 0.0f || std::abs(m_lineSpacing - 1.0f) > 0.001f;
    if (customLead) {
        GraphicsContext::TextLayoutOptions opt;
        opt.lineSpacing = m_lineSpacing;
        opt.lineHeight = m_lineHeight;
        auto layout = GraphicsContext::CreateTextLayout(Utf8ToUtf16(text), font, fontSize, opt, weight, ResolveFontStyle(), ResolveFontStretch());
        DWRITE_TEXT_METRICS metrics{};
        if (layout && SUCCEEDED(layout->GetMetrics(&metrics))) {
            measured = Size(metrics.widthIncludingTrailingWhitespace, metrics.height);
        } else {
            measured = ctx.MeasureText(text, font, fontSize, weight);
        }
    } else {
        measured = ctx.MeasureText(text, font, fontSize, weight);
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

    D2D1_COLOR_F color = m_hasColorValue
        ? m_colorValue
        : ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();
    DWRITE_FONT_WEIGHT weight = ResolveFontWeight();

    DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING;
    if (GetTextAlign() == TextAlignment::Center) align = DWRITE_TEXT_ALIGNMENT_CENTER;
    else if (GetTextAlign() == TextAlignment::Right) align = DWRITE_TEXT_ALIGNMENT_TRAILING;

    DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    if (GetVerticalAlign() == TextVerticalAlignment::Top) vAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    else if (GetVerticalAlign() == TextVerticalAlignment::Bottom) vAlign = DWRITE_PARAGRAPH_ALIGNMENT_FAR;

    Thickness padding = GetPadding();
    Rect textRect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        m_bounds.width - padding.left - padding.right,
        m_bounds.height - padding.top - padding.bottom
    );

    const bool customLead = m_lineHeight > 0.0f || std::abs(m_lineSpacing - 1.0f) > 0.001f;
    if (customLead) {
        GraphicsContext::TextLayoutOptions opt;
        opt.maxWidth = textRect.width;
        opt.maxHeight = textRect.height;
        opt.wrapping = DWRITE_WORD_WRAPPING_WRAP;
        opt.paragraphAlignment = vAlign;
        opt.lineSpacing = m_lineSpacing;
        opt.lineHeight = m_lineHeight;
        auto layout = GraphicsContext::CreateTextLayout(Utf8ToUtf16(text), font, fontSize, opt, weight, ResolveFontStyle(), ResolveFontStretch());
        if (layout) {
            layout->SetTextAlignment(align);
            const DWRITE_TEXT_RANGE range = { 0, static_cast<UINT32>(Utf8ToUtf16(text).length()) };
            layout->SetUnderline(IsUnderline(), range);
            layout->SetStrikethrough(IsStrikethrough(), range);
            ctx.DrawTextLayout(layout.Get(), textRect, color);
            return;
        }
    }
    ctx.DrawText(text, textRect, color, font, fontSize, align, vAlign, weight, false, ResolveFontStyle(), ResolveFontStretch(), IsUnderline(), IsStrikethrough());
}

} // namespace CUI
