#include "TabView.h"
#include <algorithm>

namespace CUI {

TabView::TabView() {
    SetProperty("background", Value("#1E1E1E"));
    SetProperty("headerBackground", Value("#252526"));
    SetProperty("activeTabBackground", Value("#1E1E1E"));
    SetProperty("inactiveTabBackground", Value("#2D2D2D"));
    SetProperty("minTabWidth", Value(80.0f));
    SetProperty("maxTabWidth", Value(260.0f));
}

std::vector<PropertyMeta> TabView::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "minTabWidth", "最小标签宽度 (MinTabWidth)", "标签栏配置", "number" });
    metas.push_back({ "maxTabWidth", "最大标签宽度 (MaxTabWidth)", "标签栏配置", "number" });
    return metas;
}

void TabView::AddTab(const std::string& title, std::shared_ptr<UIElement> content, const std::string& icon, bool isClosable) {
    TabViewItem item;
    item.title = title;
    item.icon = icon;
    item.content = content;
    item.isClosable = isClosable;

    if (content) {
        AddChild(content);
        int addedIndex = static_cast<int>(m_tabs.size());
        if (addedIndex != m_selectedIndex) {
            content->SetProperty("visibility", Value("Collapsed"));
        }
    }

    m_tabs.push_back(item);

    if (m_tabs.size() == 1) {
        SetSelectedIndex(0);
    }
}

void TabView::RemoveTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;

    if (m_tabs[index].content) {
        RemoveChild(m_tabs[index].content);
    }

    m_tabs.erase(m_tabs.begin() + index);

    if (m_selectedIndex >= static_cast<int>(m_tabs.size())) {
        m_selectedIndex = static_cast<int>(m_tabs.size()) - 1;
    }

    if (m_selectedIndex >= 0) {
        SetSelectedIndex(m_selectedIndex);
    }

    m_tabClosedEvent.Invoke(this, index);
}

void TabView::SetSelectedIndex(int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    if (m_selectedIndex == index && m_tabs.size() > 1) return;

    m_selectedIndex = index;

    // Show active tab content, hide other contents
    float headerH = 36.0f;
    Rect contentRect(m_bounds.x, m_bounds.y + headerH, m_bounds.width, m_bounds.height - headerH);
    Size contentAvail(m_bounds.width, m_bounds.height - headerH);

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].content) {
            if (static_cast<int>(i) == m_selectedIndex) {
                m_tabs[i].content->SetProperty("visibility", Value("Visible"));
                m_tabs[i].content->Measure(contentAvail);
                m_tabs[i].content->Arrange(contentRect);
            } else {
                m_tabs[i].content->SetProperty("visibility", Value("Collapsed"));
            }
        }
    }

    m_selectionChangedEvent.Invoke(this, m_selectedIndex);
}

Size TabView::Measure(Size availableSize) {
    float headerH = 36.0f;
    Size contentAvail(availableSize.width, availableSize.height - headerH);

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].content) {
            m_tabs[i].content->Measure(contentAvail);
        }
    }

    m_desiredSize = availableSize;
    return m_desiredSize;
}

void TabView::Arrange(Rect finalRect) {
    m_bounds = finalRect;
    float headerH = 36.0f;
    Rect contentRect(finalRect.x, finalRect.y + headerH, finalRect.width, finalRect.height - headerH);

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].content) {
            m_tabs[i].content->Arrange(contentRect);
        }
    }
}

void TabView::OnRender(GraphicsContext& ctx) {
    float headerH = 36.0f;
    Rect headerBarRect(m_bounds.x, m_bounds.y, m_bounds.width, headerH);

    // Draw TabBar Header background
    D2D1_COLOR_F headerBg = GetProperty("headerBackground").AsColor(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f));
    ctx.FillRect(headerBarRect, headerBg);

    // Clip tab buttons within header bounds
    ctx.PushClip(headerBarRect);

    float tabX = m_bounds.x + 4.0f - m_scrollOffsetX;

    float minW = GetProperty("minTabWidth").AsFloat(80.0f);
    float maxW = GetProperty("maxTabWidth").AsFloat(260.0f);

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        bool isActive = (static_cast<int>(i) == m_selectedIndex);

        Size titleSize = ctx.MeasureText(tab.title, "Segoe UI", 12.0f);
        float neededW = titleSize.width + 54.0f;
        float tabW = std::clamp(neededW, minW, maxW);
        Rect tabRect(tabX, m_bounds.y + 4.0f, tabW, headerH - 4.0f);

        D2D1_COLOR_F tabBg = isActive ? GetProperty("activeTabBackground").AsColor(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f))
                                      : GetProperty("inactiveTabBackground").AsColor(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f));

        // Draw tab background with rounded top corners
        ctx.FillRoundedRect(tabRect, 4.0f, tabBg);

        if (isActive) {
            // Active accent indicator bar at top
            ctx.FillRect(Rect(tabX + 4, m_bounds.y + 4.0f, tabW - 8, 2.0f), D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f));
        }

        // Draw Icon if available
        float titleX = tabX + 12.0f;
        if (!tab.icon.empty()) {
            ctx.DrawText(tab.icon, Rect(titleX, tabRect.y, 16, tabRect.height), D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            titleX += 20.0f;
        }

        // Draw Title Text
        D2D1_COLOR_F textColor = isActive ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0x99 / 255.0f, 0x99 / 255.0f, 0x99 / 255.0f);
        ctx.DrawText(tab.title, Rect(titleX, tabRect.y, tabW - (titleX - tabX) - 24.0f, tabRect.height), textColor, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isActive ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);

        // Draw Close 'X' Button if closable
        if (tab.isClosable) {
            float closeX = tabX + tabW - 22.0f;
            float closeY = tabRect.y + (tabRect.height - 16.0f) * 0.5f;
            Rect closeRect(closeX, closeY, 16.0f, 16.0f);

            bool isCloseHover = (m_hoveredCloseIndex == static_cast<int>(i));
            if (isCloseHover) {
                ctx.FillRoundedRect(closeRect, 3.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.15f));
            }

            D2D1_COLOR_F closeColor = isCloseHover ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0x88 / 255.0f, 0x88 / 255.0f, 0x88 / 255.0f);
            float cx = closeX + 8.0f;
            float cy = closeY + 8.0f;
            ctx.DrawLine(Point(cx - 3.5f, cy - 3.5f), Point(cx + 3.5f, cy + 3.5f), closeColor, 1.2f);
            ctx.DrawLine(Point(cx + 3.5f, cy - 3.5f), Point(cx - 3.5f, cy + 3.5f), closeColor, 1.2f);
        }

        tabX += tabW + 4.0f;
    }

    ctx.PopClip();

    // Draw TabBar Bottom border line
    ctx.DrawLine(Point(m_bounds.x, m_bounds.y + headerH - 1), Point(m_bounds.x + m_bounds.width, m_bounds.y + headerH - 1), D2D1::ColorF(0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f));
}

void TabView::OnMouseWheel(float delta) {
    float headerH = 36.0f;
    GraphicsContext ctx;
    float totalTabsWidth = 8.0f;
    float minW = GetProperty("minTabWidth").AsFloat(80.0f);
    float maxW = GetProperty("maxTabWidth").AsFloat(260.0f);

    for (const auto& tab : m_tabs) {
        Size titleSize = ctx.MeasureText(tab.title, "Segoe UI", 12.0f);
        float neededW = titleSize.width + 54.0f;
        totalTabsWidth += std::clamp(neededW, minW, maxW) + 4.0f;
    }

    float maxScroll = (std::max)(0.0f, totalTabsWidth - m_bounds.width);
    m_scrollOffsetX -= delta * 40.0f;
    if (m_scrollOffsetX < 0.0f) m_scrollOffsetX = 0.0f;
    if (m_scrollOffsetX > maxScroll) m_scrollOffsetX = maxScroll;
}

void TabView::OnMouseMove(Point pt) {
    float headerH = 36.0f;
    int oldHover = m_hoveredCloseIndex;
    m_hoveredCloseIndex = -1;

    if (pt.y >= m_bounds.y && pt.y <= m_bounds.y + headerH) {
        GraphicsContext ctx;
        float tabX = m_bounds.x + 4.0f - m_scrollOffsetX;
        float minW = GetProperty("minTabWidth").AsFloat(80.0f);
        float maxW = GetProperty("maxTabWidth").AsFloat(260.0f);

        for (size_t i = 0; i < m_tabs.size(); ++i) {
            Size titleSize = ctx.MeasureText(m_tabs[i].title, "Segoe UI", 12.0f);
            float neededW = titleSize.width + 54.0f;
            float tabW = std::clamp(neededW, minW, maxW);

            if (m_tabs[i].isClosable) {
                float closeX = tabX + tabW - 22.0f;
                float closeY = m_bounds.y + 4.0f + (headerH - 4.0f - 16.0f) * 0.5f;
                Rect closeRect(closeX, closeY, 16.0f, 16.0f);
                if (closeRect.Contains(pt.x, pt.y)) {
                    m_hoveredCloseIndex = static_cast<int>(i);
                    break;
                }
            }

            tabX += tabW + 4.0f;
        }
    }
}

void TabView::OnMouseDown(Point pt) {
    UIElement::OnMouseDown(pt);

    float headerH = 36.0f;
    if (pt.y >= m_bounds.y && pt.y <= m_bounds.y + headerH) {
        GraphicsContext ctx;
        float tabX = m_bounds.x + 4.0f - m_scrollOffsetX;
        float minW = GetProperty("minTabWidth").AsFloat(80.0f);
        float maxW = GetProperty("maxTabWidth").AsFloat(260.0f);

        for (size_t i = 0; i < m_tabs.size(); ++i) {
            Size titleSize = ctx.MeasureText(m_tabs[i].title, "Segoe UI", 12.0f);
            float neededW = titleSize.width + 54.0f;
            float tabW = std::clamp(neededW, minW, maxW);
            Rect tabRect(tabX, m_bounds.y + 4.0f, tabW, headerH - 4.0f);

            if (tabRect.Contains(pt.x, pt.y)) {
                // Check if close button was clicked
                if (m_tabs[i].isClosable) {
                    float closeX = tabX + tabW - 22.0f;
                    float closeY = m_bounds.y + 4.0f + (headerH - 4.0f - 16.0f) * 0.5f;
                    Rect closeRect(closeX, closeY, 16.0f, 16.0f);
                    if (closeRect.Contains(pt.x, pt.y)) {
                        RemoveTab(static_cast<int>(i));
                        return;
                    }
                }

                SetSelectedIndex(static_cast<int>(i));
                break;
            }

            tabX += tabW + 4.0f;
        }
    }
}

} // namespace CUI
