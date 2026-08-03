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
    SetProperty("theme.backgroundToken", Value("cardBackground"));
    SetProperty("theme.borderToken", Value("cardBorder"));
    SetProperty("theme.colorToken", Value("textSecondary"));
    SetProperty("theme.activeColorToken", Value("accentColor"));
    SetProperty("background", Value(ThemeManager::Instance().GetColor("cardBackground")));
    SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor("cardBorder")));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("color", Value(ThemeManager::Instance().GetColor("textSecondary")));
    SetProperty("activeColor", Value(ThemeManager::Instance().GetColor("accentColor")));
    SetProperty("cornerRadius", Value(4.0f));
    SetProperty("width", Value(320.0f));
    SetProperty("height", Value(30.0f));
}

std::vector<PropertyMeta> BreadcrumbBar::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    return metas;
}

Size BreadcrumbBar::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(320.0f);
    float expH = GetProperty("height").AsFloat(30.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void BreadcrumbBar::SetPath(const std::vector<std::string>& pathNodes) {
    m_pathNodes = pathNodes;
}

void BreadcrumbBar::PushNode(const std::string& node) {
    m_pathNodes.push_back(node);
}

void BreadcrumbBar::PopNode() {
    if (!m_pathNodes.empty()) {
        m_pathNodes.pop_back();
    }
}

void BreadcrumbBar::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    GraphicsContext ctx;
    float currX = m_bounds.x + 8.0f;
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontH = GetProperty("fontSize").AsFloat(12.0f);

    for (size_t i = 0; i < m_pathNodes.size(); ++i) {
        Size sz = ctx.MeasureText(m_pathNodes[i], font, fontH);
        Rect itemRect(currX, m_bounds.y, sz.width + 12.0f, m_bounds.height);
        if (itemRect.Contains(pt.x, pt.y)) {
            m_pathNodes.resize(i + 1);
            m_onItemClickedEvent.Invoke(this, static_cast<int>(i), m_pathNodes[i]);
            break;
        }
        currX += sz.width + 24.0f;
    }
}

void BreadcrumbBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    float currX = m_bounds.x + 8.0f;
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontH = GetProperty("fontSize").AsFloat(12.0f);
    D2D1_COLOR_F textColor = ResolveThemeColor("theme.colorToken", "textSecondary");
    D2D1_COLOR_F activeColor = ResolveThemeColor("theme.activeColorToken", "accentColor");
    D2D1_COLOR_F sepColor = ThemeManager::Instance().GetTokens().textMuted;

    for (size_t i = 0; i < m_pathNodes.size(); ++i) {
        bool isLast = (i == m_pathNodes.size() - 1);
        Size sz = ctx.MeasureText(m_pathNodes[i], font, fontH);
        Rect itemRect(currX, m_bounds.y, sz.width + 8.0f, m_bounds.height);

        ctx.DrawText(m_pathNodes[i], itemRect, isLast ? activeColor : textColor, font, fontH, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isLast ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
        currX += sz.width + 12.0f;

        if (!isLast) {
            Rect sepRect(currX, m_bounds.y, 12.0f, m_bounds.height);
            ctx.DrawText(">", sepRect, sepColor, font, fontH, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            currX += 12.0f;
        }
    }
}

} // namespace CUI
