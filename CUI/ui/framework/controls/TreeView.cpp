#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "TreeView.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {

TreeView::TreeView() {
    SetProperty("background", Value(ThemeManager::Instance().GetColor("cardBackground")));
    SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor("cardBorder")));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("color", Value(ThemeManager::Instance().GetColor("textPrimary")));
    SetProperty("selectedBackground", Value(ThemeManager::Instance().GetColor("selectedBackground")));
    SetProperty("hoverBackground", Value(ThemeManager::Instance().GetColor("hoverBackground")));
    SetProperty("fontSize", Value(13.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("itemHeight", Value(24.0f));
    SetProperty("indentWidth", Value(18.0f));
    SetProperty("cornerRadius", Value(4.0f));
    SetProperty("width", Value(260.0f));
    SetProperty("height", Value(340.0f));
}

std::vector<PropertyMeta> TreeView::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "Segoe UI", "Consolas", "微软雅黑", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "color", "文字颜色 (Color)", "字体文本", "color" });
    metas.push_back({ "itemHeight", "节点行高 (ItemHeight)", "树形配置", "number" });
    metas.push_back({ "indentWidth", "层级缩进 (IndentWidth)", "树形配置", "number" });
    metas.push_back({ "selectedBackground", "选中背景色 (SelBg)", "色彩外观", "color" });
    metas.push_back({ "hoverBackground", "悬停背景色 (HoverBg)", "色彩外观", "color" });
    return metas;
}

HCURSOR TreeView::GetCursor() const {
    return IsEnabled() ? LoadCursor(nullptr, IDC_ARROW) : nullptr;
}

void TreeView::SetParentRecursive(const std::shared_ptr<TreeViewItem>& item, TreeViewItem* parent) {
    if (!item) return;
    item->parent = parent;
    for (auto& child : item->children) {
        SetParentRecursive(child, item.get());
    }
}

void TreeView::ClearItems() {
    m_items.clear();
    m_selectedItem = nullptr;
    m_visibleItems.clear();
    m_visibleDirty = true;
    m_scrollY = 0.0f;
    m_hoveredVisibleIndex = -1;
    m_pressedVisibleIndex = -1;
}

void TreeView::AddItem(std::shared_ptr<TreeViewItem> item) {
    if (item) {
        SetParentRecursive(item, nullptr);
        m_items.push_back(item);
        m_visibleDirty = true;
    }
}

std::shared_ptr<TreeViewItem> TreeView::AddItem(const std::string& header, bool expanded) {
    auto item = std::make_shared<TreeViewItem>();
    item->header = header;
    item->isExpanded = expanded;
    AddItem(item);
    return item;
}

void TreeView::SetItems(const std::vector<std::shared_ptr<TreeViewItem>>& items) {
    m_items = items;
    for (auto& item : m_items) {
        SetParentRecursive(item, nullptr);
    }
    m_selectedItem = nullptr;
    m_visibleDirty = true;
    m_scrollY = 0.0f;
    m_hoveredVisibleIndex = -1;
    m_pressedVisibleIndex = -1;
}

void TreeView::RebuildVisibleItems() const {
    if (!m_visibleDirty) return;
    m_visibleItems.clear();

    auto collect = [this](auto self, const std::vector<std::shared_ptr<TreeViewItem>>& list, int depth) -> void {
        for (const auto& item : list) {
            if (!item) continue;
            m_visibleItems.push_back({ item, depth });
            if (item->isExpanded && !item->children.empty()) {
                self(self, item->children, depth + 1);
            }
        }
    };

    collect(collect, m_items, 0);
    m_visibleDirty = false;
}

void TreeView::ClampScroll() {
    RebuildVisibleItems();
    float itemH = GetItemHeight();
    float contentH = m_visibleItems.size() * itemH;
    float viewportH = (std::max)(0.0f, m_bounds.height - 4.0f);
    float maxScroll = (std::max)(0.0f, contentH - viewportH);
    m_scrollY = std::clamp(m_scrollY, 0.0f, maxScroll);
}

int TreeView::GetVisibleIndexFromY(float y) const {
    RebuildVisibleItems();
    if (m_visibleItems.empty() || y < m_bounds.y + 2.0f || y > m_bounds.y + m_bounds.height - 2.0f) {
        return -1;
    }
    float itemH = GetItemHeight();
    float relY = (y - (m_bounds.y + 2.0f)) + m_scrollY;
    int idx = static_cast<int>(std::floor(relY / itemH));
    if (idx >= 0 && idx < static_cast<int>(m_visibleItems.size())) {
        return idx;
    }
    return -1;
}

int TreeView::GetVisibleIndexOfItem(TreeViewItem* item) const {
    if (!item) return -1;
    RebuildVisibleItems();
    for (size_t i = 0; i < m_visibleItems.size(); ++i) {
        if (m_visibleItems[i].item.get() == item) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Rect TreeView::GetItemRect(int visibleIndex) const {
    if (visibleIndex < 0) return Rect(0, 0, 0, 0);
    float itemH = GetItemHeight();
    float itemY = m_bounds.y + 2.0f + visibleIndex * itemH - m_scrollY;
    return Rect(m_bounds.x + 2.0f, itemY, (std::max)(0.0f, m_bounds.width - 4.0f), itemH);
}

Rect TreeView::GetToggleRect(const VisibleItem& visibleItem, const Rect& rowRect) const {
    float indentW = GetIndentWidth();
    float toggleX = rowRect.x + visibleItem.depth * indentW + 4.0f;
    float toggleY = rowRect.y + (rowRect.height - 14.0f) * 0.5f;
    return Rect(toggleX, toggleY, 14.0f, 14.0f);
}

void TreeView::ToggleItem(std::shared_ptr<TreeViewItem> item) {
    if (!item || item->children.empty()) return;
    item->isExpanded = !item->isExpanded;
    m_visibleDirty = true;
    ClampScroll();
    m_onItemToggledEvent.Invoke(this, item);
}

void TreeView::SetSelectedItem(std::shared_ptr<TreeViewItem> item) {
    if (m_selectedItem != item) {
        if (m_selectedItem) m_selectedItem->isSelected = false;
        m_selectedItem = item;
        if (m_selectedItem) m_selectedItem->isSelected = true;

        // Auto expand ancestors so selected item remains visible
        if (m_selectedItem) {
            TreeViewItem* p = m_selectedItem->parent;
            while (p) {
                p->isExpanded = true;
                p = p->parent;
            }
            m_visibleDirty = true;
        }

        ClampScroll();
        m_onSelectionChangedEvent.Invoke(this, m_selectedItem);
    }
}

Size TreeView::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(260.0f);
    float expH = GetProperty("height").AsFloat(340.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

UIElement* TreeView::HitTest(float x, float y) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return nullptr;
    if (m_bounds.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void TreeView::Render(GraphicsContext& ctx) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return;

    ctx.PushClip(m_bounds);
    OnRender(ctx);
    ctx.PopClip();
}

void TreeView::OnRender(GraphicsContext& ctx) {
    // Draw TreeView Container Background & Border (Do not call Control::OnRender to prevent m_isPressed from darkening whole container)
    float radius = GetProperty("cornerRadius").AsFloat(4.0f);
    D2D1_COLOR_F bg = GetProperty("background").AsColor(ThemeManager::Instance().GetColor("cardBackground"));
    D2D1_COLOR_F border = GetProperty("borderBrush").AsColor(ThemeManager::Instance().GetColor("cardBorder"));
    float borderThick = GetProperty("borderThickness").AsFloat(1.0f);

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, borderThick);

    RebuildVisibleItems();

    float itemH = GetItemHeight();
    float indentW = GetIndentWidth();
    std::string fontFamily = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);
    D2D1_COLOR_F textColor = GetProperty("color").AsColor(ThemeManager::Instance().GetColor("textPrimary"));
    D2D1_COLOR_F selBg = GetProperty("selectedBackground").AsColor(ThemeManager::Instance().GetColor("selectedBackground"));
    D2D1_COLOR_F hoverBg = GetProperty("hoverBackground").AsColor(ThemeManager::Instance().GetColor("hoverBackground"));

    for (size_t i = 0; i < m_visibleItems.size(); ++i) {
        const auto& visItem = m_visibleItems[i];
        const auto& item = visItem.item;
        Rect rowRect = GetItemRect(static_cast<int>(i));

        // Skip rows outside viewport
        if (rowRect.y + rowRect.height < m_bounds.y || rowRect.y > m_bounds.y + m_bounds.height) {
            continue;
        }

        bool isSelected = (m_selectedItem == item);
        bool isHovered = (m_hoveredVisibleIndex == static_cast<int>(i));

        // Render Row Background
        if (isSelected) {
            ctx.FillRoundedRect(rowRect, 3.0f, selBg);
        } else if (isHovered) {
            ctx.FillRoundedRect(rowRect, 3.0f, hoverBg);
        }

        float currX = rowRect.x + visItem.depth * indentW + 4.0f;

        // Render Expand / Collapse Arrow
        if (!item->children.empty()) {
            Rect toggleRect = GetToggleRect(visItem, rowRect);
            float cx = toggleRect.x + toggleRect.width * 0.5f;
            float cy = toggleRect.y + toggleRect.height * 0.5f;
            D2D1_COLOR_F arrowColor = ThemeManager::Instance().GetColor("textMuted");

            if (item->isExpanded) {
                // Down Arrow (Expanded)
                ctx.DrawLine(Point(cx - 3.5f, cy - 2.0f), Point(cx, cy + 2.5f), arrowColor, 1.4f);
                ctx.DrawLine(Point(cx, cy + 2.5f), Point(cx + 3.5f, cy - 2.0f), arrowColor, 1.4f);
            } else {
                // Right Arrow (Collapsed)
                ctx.DrawLine(Point(cx - 2.0f, cy - 3.5f), Point(cx + 2.5f, cy), arrowColor, 1.4f);
                ctx.DrawLine(Point(cx + 2.5f, cy), Point(cx - 2.0f, cy + 3.5f), arrowColor, 1.4f);
            }
        }
        currX += 16.0f;

        // Render Icon (or folder/file glyph fallback)
        std::string iconText = item->icon;
        if (iconText.empty()) {
            iconText = !item->children.empty() ? (item->isExpanded ? "📂" : "📁") : "📄";
        }
        Rect iconRect(currX, rowRect.y, 16.0f, rowRect.height);
        ctx.DrawText(iconText, iconRect, ThemeManager::Instance().GetColor("accentColor"), "Segoe UI Emoji", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        currX += 20.0f;

        // Render Header Text
        float textW = (std::max)(0.0f, rowRect.x + rowRect.width - currX - 4.0f);
        Rect textRect(currX, rowRect.y, textW, rowRect.height);
        ctx.DrawText(item->header, textRect, isSelected ? ThemeManager::Instance().GetColor("textPrimary") : textColor, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
    }

    // Scrollbar indicator
    float contentH = m_visibleItems.size() * itemH;
    float viewportH = m_bounds.height;
    if (contentH > viewportH && viewportH > 0.0f) {
        float maxScroll = contentH - viewportH;
        float trackH = viewportH - 4.0f;
        float thumbH = (viewportH / contentH) * trackH;
        if (thumbH < 20.0f) thumbH = 20.0f;

        float scrollRatio = m_scrollY / maxScroll;
        float thumbY = m_bounds.y + 2.0f + scrollRatio * (trackH - thumbH);
        Rect thumbRect(m_bounds.x + m_bounds.width - 6.0f, thumbY, 4.0f, thumbH);
        ctx.FillRoundedRect(thumbRect, 2.0f, D2D1::ColorF(border.r, border.g, border.b, 0.4f));
    }
}

void TreeView::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    m_isMouseDown = true;
    int idx = GetVisibleIndexFromY(pt.y);
    if (idx >= 0 && idx < static_cast<int>(m_visibleItems.size())) {
        const auto& visItem = m_visibleItems[idx];
        Rect rowRect = GetItemRect(idx);

        // Check if expander arrow clicked
        if (!visItem.item->children.empty()) {
            Rect toggleRect = GetToggleRect(visItem, rowRect);
            if (toggleRect.Contains(pt.x, pt.y)) {
                ToggleItem(visItem.item);
                return;
            }
        }

        // Selection click
        m_pressedVisibleIndex = idx;
        SetSelectedItem(visItem.item);
    }
}

void TreeView::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    int oldHover = m_hoveredVisibleIndex;
    m_hoveredVisibleIndex = GetVisibleIndexFromY(pt.y);
}

void TreeView::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isMouseDown = false;
    m_pressedVisibleIndex = -1;
}

void TreeView::OnMouseWheel(float delta) {
    RebuildVisibleItems();
    float itemH = GetItemHeight();
    float contentH = m_visibleItems.size() * itemH;
    float viewportH = (std::max)(0.0f, m_bounds.height - 4.0f);
    float maxScroll = (std::max)(0.0f, contentH - viewportH);
    if (maxScroll <= 0.0f) {
        UIElement::OnMouseWheel(delta);
        return;
    }

    m_scrollY -= delta * 40.0f;
    ClampScroll();
}

std::shared_ptr<TreeViewItem> TreeView::FindFirstVisibleSelectable(int startIndex, int direction) const {
    RebuildVisibleItems();
    if (m_visibleItems.empty()) return nullptr;

    int idx = startIndex + direction;
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(m_visibleItems.size())) idx = static_cast<int>(m_visibleItems.size()) - 1;

    return m_visibleItems[idx].item;
}

void TreeView::OnKeyDown(int vkCode) {
    Control::OnKeyDown(vkCode);
    RebuildVisibleItems();
    if (m_visibleItems.empty()) return;

    int currIdx = GetVisibleIndexOfItem(m_selectedItem.get());

    switch (vkCode) {
    case VK_UP: {
        int nextIdx = (currIdx > 0) ? currIdx - 1 : 0;
        SetSelectedItem(m_visibleItems[nextIdx].item);
        break;
    }
    case VK_DOWN: {
        int nextIdx = (currIdx < static_cast<int>(m_visibleItems.size()) - 1) ? currIdx + 1 : static_cast<int>(m_visibleItems.size()) - 1;
        SetSelectedItem(m_visibleItems[nextIdx].item);
        break;
    }
    case VK_RIGHT: {
        if (m_selectedItem) {
            if (!m_selectedItem->children.empty()) {
                if (!m_selectedItem->isExpanded) {
                    ToggleItem(m_selectedItem);
                } else if (!m_selectedItem->children.empty()) {
                    SetSelectedItem(m_selectedItem->children[0]);
                }
            }
        }
        break;
    }
    case VK_LEFT: {
        if (m_selectedItem) {
            if (!m_selectedItem->children.empty() && m_selectedItem->isExpanded) {
                ToggleItem(m_selectedItem);
            } else if (m_selectedItem->parent) {
                // Move selection to parent
                for (const auto& vis : m_visibleItems) {
                    if (vis.item.get() == m_selectedItem->parent) {
                        SetSelectedItem(vis.item);
                        break;
                    }
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

} // namespace CUI
