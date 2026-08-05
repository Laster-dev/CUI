#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "TreeView.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {

TreeView::TreeView() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetBackground(ThemeManager::Instance().GetColor("cardBackground"));
    SetBorderBrush(ThemeManager::Instance().GetColor("cardBorder"));
    SetBorderThickness(1.0f);
    SetColor(ThemeManager::Instance().GetColor("textPrimary"));
    SetHoverBackground(ThemeManager::Instance().GetColor("hoverBackground"));
    SetFontSize(13.0f);
    SetFontFamily("Segoe UI");
    SetItemHeight(24.0f);
    SetCornerRadius(4.0f);
    SetWidth(260.0f);
    SetHeight(340.0f);
}

std::vector<PropertyMeta> TreeView::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "Segoe UI", "Consolas", "微软雅黑", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "itemHeight", "节点行高 (ItemHeight)", "树形配置", "number" });
    metas.push_back({ "indentWidth", "层级缩进 (IndentWidth)", "树形配置", "number" });
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
    item->expandAnim.Reset(expanded ? 1.0f : 0.0f);
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
            // Keep children visible while collapse anim is running.
            const bool showChildren = item->isExpanded || item->expandAnim.Current() > 0.01f;
            if (showChildren && !item->children.empty()) {
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
    item->expandAnim.SetTarget(item->isExpanded ? 1.0f : 0.0f);
    if (!UIElement::AreAnimationsEnabled()) {
        item->expandAnim.Reset(item->isExpanded ? 1.0f : 0.0f);
    } else {
        RequestAnimationTicks();
    }
    m_visibleDirty = true;
    ClampScroll();
    MarkRenderContentDirty();
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
                p->expandAnim.Reset(1.0f);
                p = p->parent;
            }
            m_visibleDirty = true;
        }

        ClampScroll();
        m_onSelectionChangedEvent.Invoke(this, m_selectedItem);
    }
}

Size TreeView::Measure(Size availableSize) {
    float expW = GetWidth(); if (expW < 0) expW = 260.0f;
    float expH = GetHeight(); if (expH < 0) expH = 340.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

UIElement* TreeView::HitTest(float x, float y) {
    if (GetVisibility() != Visibility::Visible) return nullptr;
    if (m_bounds.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void TreeView::Render(GraphicsContext& ctx) {
    if (GetVisibility() != Visibility::Visible) return;

    ctx.PushClip(m_bounds);
    OnRender(ctx);
    ctx.PopClip();
}

void TreeView::OnRender(GraphicsContext& ctx) {
    // Draw TreeView Container Background & Border (Do not call Control::OnRender to prevent m_isPressed from darkening whole container)
    float radius = GetCornerRadius();
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    float borderThick = GetBorderThickness();

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, borderThick);

    RebuildVisibleItems();

    float itemH = GetItemHeight();
    float indentW = GetIndentWidth();
    std::string fontFamily = GetFontFamily();
    float fontSize = GetFontSize();
    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    D2D1_COLOR_F selBg = ResolveThemeColor(GetSelectedBackgroundToken(), ThemeTokenId::SelectedBackground);
    D2D1_COLOR_F hoverBg = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground);

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

        // Render Expand / Collapse Arrow (animated via expandAnim).
        if (!item->children.empty()) {
            Rect toggleRect = GetToggleRect(visItem, rowRect);
            const float progress = std::clamp(item->expandAnim.Current(), 0.0f, 1.0f);
            D2D1_COLOR_F arrowColor = ThemeManager::Instance().GetColor(ThemeTokenId::TextMuted);
            ctx.DrawChevron(
                toggleRect,
                arrowColor,
                progress > 0.5f ? GraphicsContext::ChevronDirection::Down
                                : GraphicsContext::ChevronDirection::Right,
                1.6f
            );
        }
        currX += 16.0f;

        // Render Icon (or folder/file glyph fallback)
        std::string iconText = item->icon;
        if (iconText.empty()) {
            iconText = !item->children.empty() ? (item->expandAnim.Current() > 0.5f ? "📂" : "📁") : "📄";
        }
        Rect iconRect(currX, rowRect.y, 16.0f, rowRect.height);
        // Fade child rows with parent expand progress.
        float rowAlpha = 1.0f;
        if (item->parent) {
            rowAlpha = std::clamp(item->parent->expandAnim.Current(), 0.0f, 1.0f);
            const float inv = 1.0f - rowAlpha;
            rowAlpha = 1.0f - inv * inv * inv;
        }
        D2D1_COLOR_F iconColor = ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor);
        iconColor.a *= rowAlpha;
        ctx.DrawText(iconText, iconRect, iconColor, "Segoe UI Emoji", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        currX += 20.0f;

        // Render Header Text
        float textW = (std::max)(0.0f, rowRect.x + rowRect.width - currX - 4.0f);
        Rect textRect(currX, rowRect.y, textW, rowRect.height);
        D2D1_COLOR_F rowText = isSelected ? ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary) : textColor;
        rowText.a *= rowAlpha;
        ctx.DrawText(item->header, textRect, rowText, fontFamily, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
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

bool TreeView::TickExpandAnims(const std::vector<std::shared_ptr<TreeViewItem>>& list, float dt) {
    bool any = false;
    for (const auto& item : list) {
        if (!item) continue;
        item->expandAnim.SetTarget(item->isExpanded ? 1.0f : 0.0f);
        if (item->expandAnim.Tick(dt, AnimationSpec{ 0.32f, 0.01f })) {
            any = true;
        }
        if (TickExpandAnims(item->children, dt)) {
            any = true;
        }
    }
    return any;
}

bool TreeView::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    if (!UIElement::AreAnimationsEnabled()) {
        return base;
    }
    const float dt = UIElement::GetAnimationDeltaSeconds();
    const bool moving = TickExpandAnims(m_items, dt);
    if (moving) {
        m_visibleDirty = true;
        ClampScroll();
        MarkRenderContentDirty();
        RequestAnimationTicks();
    }
    return base || moving;
}

bool TreeView::HasSelfAnimation() const {
    auto walk = [](auto self, const std::vector<std::shared_ptr<TreeViewItem>>& list) -> bool {
        for (const auto& item : list) {
            if (!item) continue;
            if (item->expandAnim.IsAnimating(0.01f)) return true;
            if (self(self, item->children)) return true;
        }
        return false;
    };
    return Control::HasSelfAnimation() || walk(walk, m_items);
}

} // namespace CUI
