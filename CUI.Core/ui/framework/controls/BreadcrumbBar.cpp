#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "BreadcrumbBar.h"
#include "../style/ThemeManager.h"
#include <sstream>
#include <algorithm>

namespace CUI {

BreadcrumbBar::BreadcrumbBar() {
    m_pathNodes = { "Home", "Controls", "BreadcrumbBar" };
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextSecondary);
    SetActiveColorToken(ThemeTokenId::TextPrimary);
    SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::PaneBackground));
    SetBorderBrush(ThemeManager::Instance().GetColor(ThemeTokenId::CardBorder));
    SetBorderThickness(1.0f);
    SetColor(ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary));
    SetCornerRadius(0.0f);
    SetFontFamily("微软雅黑");
    SetFontSize(16.0f);
    SetWidth(-1.0f);
    SetHeight(34.0f);
}

std::vector<PropertyMeta> BreadcrumbBar::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    return metas;
}

Size BreadcrumbBar::Measure(Size availableSize) {
    float expW = GetWidth();
    float expH = GetHeight();
    if (expW < 0.0f) {
        expW = (availableSize.width > 0.0f && availableSize.width < 1.0e6f)
                   ? availableSize.width
                   : 320.0f;
    }
    if (expH < 0.0f) expH = 34.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void BreadcrumbBar::SetPath(const std::vector<std::string>& pathNodes) {
    m_pathNodes = pathNodes;
    MarkRenderContentDirty();
}

void BreadcrumbBar::PushNode(const std::string& node) {
    m_pathNodes.push_back(node);
    MarkRenderContentDirty();
}

void BreadcrumbBar::PopNode() {
    if (!m_pathNodes.empty()) {
        m_pathNodes.pop_back();
        MarkRenderContentDirty();
    }
}

void BreadcrumbBar::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    GraphicsContext ctx;
    float currX = m_bounds.x + 8.0f;
    const std::string& font = GetFontFamily();
    float fontH = GetFontSize();
    constexpr float kGap = 4.0f;
    constexpr float kTextPad = 2.0f;

    for (size_t i = 0; i < m_pathNodes.size(); ++i) {
        const bool isLast = (i + 1 == m_pathNodes.size());
        const DWRITE_FONT_WEIGHT weight = isLast ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        Size sz = ctx.MeasureText(m_pathNodes[i], font, fontH, weight);
        Rect itemRect(currX, m_bounds.y, sz.width + kTextPad, m_bounds.height);
        if (itemRect.Contains(pt.x, pt.y)) {
            // Do not mutate path here — the host decides navigation.
            m_onItemClickedEvent.Invoke(this, static_cast<int>(i), m_pathNodes[i]);
            break;
        }
        currX += sz.width + kTextPad + kGap;
        if (!isLast) {
            Size sep = ctx.MeasureText("\\", font, fontH, DWRITE_FONT_WEIGHT_NORMAL);
            currX += sep.width + kGap;
        }
    }
}

void BreadcrumbBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    // Bottom hairline under the address/breadcrumb strip.
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    ctx.DrawLine(
        Point(m_bounds.x, m_bounds.y + m_bounds.height - 1.0f),
        Point(m_bounds.x + m_bounds.width, m_bounds.y + m_bounds.height - 1.0f),
        border,
        1.0f
    );

    float currX = m_bounds.x + 8.0f;
    const std::string& font = GetFontFamily();
    float fontH = GetFontSize();
    constexpr float kGap = 4.0f;
    constexpr float kTextPad = 2.0f; // DWrite layout can be slightly wider than metrics floor
    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextSecondary);
    D2D1_COLOR_F activeColor = ResolveThemeColor(GetActiveColorToken(), ThemeTokenId::TextPrimary);
    D2D1_COLOR_F sepColor = ThemeManager::Instance().GetTokens().textMuted;

    for (size_t i = 0; i < m_pathNodes.size(); ++i) {
        bool isLast = (i == m_pathNodes.size() - 1);
        // Last segment is SemiBold — measure with the same weight used for DrawText.
        const DWRITE_FONT_WEIGHT weight = isLast ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        Size sz = ctx.MeasureText(m_pathNodes[i], font, fontH, weight);
        Rect itemRect(currX, m_bounds.y, sz.width + kTextPad, m_bounds.height);

        ctx.DrawText(
            m_pathNodes[i],
            itemRect,
            isLast ? activeColor : textColor,
            font,
            fontH,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            weight
        );
        currX += sz.width + kTextPad + kGap;

        if (!isLast) {
            Size sep = ctx.MeasureText("\\", font, fontH, DWRITE_FONT_WEIGHT_NORMAL);
            Rect sepRect(currX, m_bounds.y, sep.width + kTextPad, m_bounds.height);
            ctx.DrawText("\\", sepRect, sepColor, font, fontH, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            currX += sep.width + kTextPad + kGap;
        }
    }
}

} // namespace CUI
