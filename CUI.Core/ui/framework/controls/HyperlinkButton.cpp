#include "../style/ThemeManager.h"
#include "HyperlinkButton.h"

namespace CUI {

HyperlinkButton::HyperlinkButton() {
    SetText("HyperlinkButton");
    SetColorToken(ThemeTokenId::AccentColor);
    SetColor(ThemeManager::Instance().GetColor("accentColor"));
    SetFontSize(13.0f);
    SetFontFamily("微软雅黑");
    SetPadding(Thickness(2, 2, 2, 2));
}

HyperlinkButton::HyperlinkButton(const std::string& text, const std::string& uri) : HyperlinkButton() {
    SetText(text);
    SetNavigateUri(uri);
}

Value HyperlinkButton::GetProperty(PropertyId id) const {
    if (id == PropertyId::NavigateUri) {
        return Value(m_navigateUri);
    }
    return Control::GetProperty(id);
}

bool HyperlinkButton::HasProperty(PropertyId id) const {
    return id == PropertyId::NavigateUri || Control::HasProperty(id);
}

void HyperlinkButton::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::NavigateUri) {
        SetNavigateUri(val.AsString());
        return;
    }
    Control::SetProperty(id, val);
}

Size HyperlinkButton::Measure(Size availableSize) {
    (void)availableSize;
    const std::string& text = GetText();
    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();

    GraphicsContext ctx;
    Size measured = ctx.MeasureText(text, font, fontSize, ResolveFontWeight());

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

void HyperlinkButton::OnRender(GraphicsContext& ctx) {
    const std::string& text = GetText();
    if (text.empty()) return;

    D2D1_COLOR_F textColor = m_hasColorValue
        ? m_colorValue
        : ResolveThemeColor(GetColorToken(), ThemeTokenId::AccentColor);

    const std::string& font = GetFontFamily();
    float fontSize = GetFontSize();

    Thickness padding = GetPadding();
    Rect textRect(
        m_bounds.x + padding.left,
        m_bounds.y + padding.top,
        m_bounds.width - padding.left - padding.right,
        m_bounds.height - padding.top - padding.bottom
    );

    ctx.DrawText(text, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, ResolveFontWeight());

    // Draw hover underline
    if (m_isHovered) {
        Size sz = ctx.MeasureText(text, font, fontSize);
        float lineY = m_bounds.y + m_bounds.height - 2.0f;
        ctx.DrawLine(Point(textRect.x, lineY), Point(textRect.x + sz.width, lineY), textColor, 1.0f);
    }
}

bool HyperlinkButton::OnKeyDown(int vkCode) {
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

} // namespace CUI
