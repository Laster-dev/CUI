#include "../style/ThemeManager.h"
#include "HyperlinkButton.h"

namespace CUI {

HyperlinkButton::HyperlinkButton() {
    SetProperty("text", Value("HyperlinkButton"));
    SetProperty("navigateUri", Value(""));
    SetProperty("theme.colorToken", Value("accentColor"));
    SetProperty("theme.hoverColorToken", Value("accentColor"));
    SetProperty("color", Value(ThemeManager::Instance().GetColor("accentColor")));
    SetProperty("hoverColor", Value(ThemeManager::Instance().GetColor("accentColor")));
    SetProperty("fontSize", Value(13.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("padding", Value(Thickness(2, 2, 2, 2)));
}

HyperlinkButton::HyperlinkButton(const std::string& text, const std::string& uri) : HyperlinkButton() {
    SetProperty("text", Value(text));
    SetProperty("navigateUri", Value(uri));
}

Size HyperlinkButton::Measure(Size availableSize) {
    std::string text = GetProperty("text").AsString("");
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);

    GraphicsContext ctx;
    Size measured = ctx.MeasureText(text, font, fontSize);

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

void HyperlinkButton::OnRender(GraphicsContext& ctx) {
    std::string text = GetProperty("text").AsString("");
    if (text.empty()) return;

    D2D1_COLOR_F textColor = ResolveThemeColor("theme.colorToken", "accentColor");
    if (m_isHovered) {
        textColor = ResolveThemeColor("theme.hoverColorToken", "accentColor");
    }

    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);

    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));
    Rect textRect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        m_bounds.width - padding.left - padding.right,
        m_bounds.height - padding.top - padding.bottom
    );

    ctx.DrawText(text, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Draw hover underline
    if (m_isHovered) {
        Size sz = ctx.MeasureText(text, font, fontSize);
        float lineY = m_bounds.y + m_bounds.height - 2.0f;
        ctx.DrawLine(Point(textRect.x, lineY), Point(textRect.x + sz.width, lineY), textColor, 1.0f);
    }
}

} // namespace CUI
