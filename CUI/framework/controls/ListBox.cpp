#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ListBox.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace CUI {

ListBox::ListBox() {
    SetProperty("background", Value(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("color", Value(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("selectedBackground", Value(D2D1::ColorF(0x04 / 255.0f, 0x39 / 255.0f, 0x61 / 255.0f, 1.0f))); // VS Code Selection Blue #043961
    SetProperty("hoverBackground", Value(D2D1::ColorF(0x2A / 255.0f, 0x2D / 255.0f, 0x2E / 255.0f, 1.0f)));
    SetProperty("fontSize", Value(13.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("itemHeight", Value(28.0f));
    SetProperty("cornerRadius", Value(4.0f));
    SetProperty("width", Value(240.0f));
    SetProperty("height", Value(300.0f));
}

void ListBox::AddItem(const std::string& item) {
    m_itemDatas.push_back({ item, nullptr });
}

void ListBox::AddItem(std::shared_ptr<UIElement> customElement) {
    if (customElement) {
        m_itemDatas.push_back({ "", customElement });
        AddChild(customElement);
    }
}

void ListBox::SetItems(const std::vector<std::string>& items) {
    m_itemDatas.clear();
    ClearChildren();
    for (const auto& s : items) {
        m_itemDatas.push_back({ s, nullptr });
    }
    m_selectedIndex = -1;
    m_scrollY = 0.0f;
}

void ListBox::ClearItems() {
    m_itemDatas.clear();
    ClearChildren();
    m_selectedIndex = -1;
    m_scrollY = 0.0f;
}

std::string ListBox::GetItemAt(size_t index) const {
    if (index < m_itemDatas.size()) {
        return m_itemDatas[index].text;
    }
    return "";
}

void ListBox::SetSelectedIndex(int index) {
    if (index >= -1 && index < static_cast<int>(m_itemDatas.size())) {
        if (m_selectedIndex != index) {
            m_selectedIndex = index;
            if (m_selectedIndex >= 0) {
                EnsureVisible(m_selectedIndex);
                m_onSelectionChangedEvent.Invoke(this, m_selectedIndex, m_itemDatas[m_selectedIndex].text);
            }
        }
    }
}

std::string ListBox::GetSelectedItem() const {
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_itemDatas.size())) {
        return m_itemDatas[m_selectedIndex].text;
    }
    return "";
}

UIElement* ListBox::HitTest(float x, float y) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return nullptr;

    if (m_bounds.Contains(x, y)) {
        int idx = GetItemIndexFromY(y);
        if (idx >= 0 && idx < static_cast<int>(m_itemDatas.size())) {
            if (m_itemDatas[idx].customElement) {
                float itemH = GetItemHeight();
                float sbWidth = (m_maxScrollY > 0.0f) ? 8.0f : 0.0f;
                float itemW = m_bounds.width - 4.0f - sbWidth;
                float itemY = m_bounds.y + 2.0f + idx * itemH - m_scrollY;
                Rect itemRect(m_bounds.x + 2.0f, itemY, itemW, itemH);

                m_itemDatas[idx].customElement->Measure(Size(itemW, itemH));
                m_itemDatas[idx].customElement->Arrange(itemRect);

                UIElement* childHit = m_itemDatas[idx].customElement->HitTest(x, y);
                if (childHit && childHit != m_itemDatas[idx].customElement.get()) return childHit;
            }
        }
        return this;
    }
    return nullptr;
}

Size ListBox::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(240.0f);
    float expH = GetProperty("height").AsFloat(300.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ListBox::ClampScroll() {
    float itemH = GetItemHeight();
    float contentH = itemH * m_itemDatas.size();
    float viewH = m_bounds.height - 4.0f;

    m_maxScrollY = std::max(0.0f, contentH - viewH);
    m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScrollY);
}

int ListBox::GetItemIndexFromY(float y) const {
    float relativeY = y - m_bounds.y - 2.0f + m_scrollY;
    float itemH = GetItemHeight();
    if (relativeY < 0.0f) return -1;
    int idx = static_cast<int>(relativeY / itemH);
    if (idx >= 0 && idx < static_cast<int>(m_itemDatas.size())) {
        return idx;
    }
    return -1;
}

void ListBox::EnsureVisible(int index) {
    if (index < 0 || index >= static_cast<int>(m_itemDatas.size())) return;

    float itemH = GetItemHeight();
    float itemTop = index * itemH;
    float itemBottom = itemTop + itemH;
    float viewH = m_bounds.height - 4.0f;

    if (itemTop < m_scrollY) {
        m_scrollY = itemTop;
    } else if (itemBottom > m_scrollY + viewH) {
        m_scrollY = itemBottom - viewH;
    }
    ClampScroll();
}

void ListBox::Render(GraphicsContext& ctx) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return;

    bool clip = ShouldClipToBounds();
    if (clip) {
        ctx.PushClip(m_bounds);
    }

    OnRender(ctx);

    if (clip) {
        ctx.PopClip();
    }
}

void ListBox::OnRender(GraphicsContext& ctx) {
    // 1. Support items property parsing from XML DSL
    std::string itemsProp = GetProperty("items").AsString("");
    if (!itemsProp.empty() && m_itemDatas.empty()) {
        std::stringstream ss(itemsProp);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) AddItem(item);
        }
    }

    ClampScroll();

    float radius = GetProperty("cornerRadius").AsFloat(4.0f);
    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f));
    D2D1_COLOR_F border = GetProperty("borderBrush").AsColor(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f));
    float borderThick = GetProperty("borderThickness").AsFloat(1.0f);

    // Draw container background and border
    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, borderThick);

    // 2. High-Performance Virtualized Item Rendering (O(Visible) instead of O(N))
    ctx.PushClip(Rect(m_bounds.x + 1, m_bounds.y + 1, m_bounds.width - 2, m_bounds.height - 2));

    float itemH = GetItemHeight();
    float fontH = GetProperty("fontSize").AsFloat(13.0f);
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    D2D1_COLOR_F textColor = GetProperty("color").AsColor(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f));
    D2D1_COLOR_F selectedBg = GetProperty("selectedBackground").AsColor(D2D1::ColorF(0x04 / 255.0f, 0x39 / 255.0f, 0x61 / 255.0f, 1.0f));
    D2D1_COLOR_F hoverBg = GetProperty("hoverBackground").AsColor(D2D1::ColorF(0x2A / 255.0f, 0x2D / 255.0f, 0x2E / 255.0f, 1.0f));

    float sbWidth = (m_maxScrollY > 0.0f) ? 8.0f : 0.0f;
    float itemW = m_bounds.width - 4.0f - sbWidth;

    // Calculate virtualized index range
    int startIdx = std::max(0, static_cast<int>(m_scrollY / itemH));
    int endIdx = std::min(static_cast<int>(m_itemDatas.size()) - 1, static_cast<int>((m_scrollY + m_bounds.height) / itemH));

    for (int i = startIdx; i <= endIdx; ++i) {
        float itemY = m_bounds.y + 2.0f + i * itemH - m_scrollY;
        Rect itemRect(m_bounds.x + 2.0f, itemY, itemW, itemH);

        bool isSelected = (i == m_selectedIndex);
        bool isHovered = (i == m_hoveredIndex);

        // Item background highlight
        if (isSelected) {
            ctx.FillRoundedRect(itemRect, 2.0f, selectedBg);
        } else if (isHovered && IsEnabled()) {
            ctx.FillRoundedRect(itemRect, 2.0f, hoverBg);
        }

        // Draw custom element OR standard text
        if (m_itemDatas[i].customElement) {
            m_itemDatas[i].customElement->Measure(Size(itemRect.width, itemRect.height));
            m_itemDatas[i].customElement->Arrange(itemRect);
            m_itemDatas[i].customElement->Render(ctx);
        } else {
            D2D1_COLOR_F textClr = isSelected ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : textColor;
            Rect textRect(itemRect.x + 8.0f, itemRect.y, itemRect.width - 16.0f, itemRect.height);
            ctx.DrawText(m_itemDatas[i].text, textRect, textClr, font, fontH, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    // 3. Render Direct2D Vector ScrollBar on Right Edge
    if (m_maxScrollY > 0.0f) {
        float trackX = m_bounds.x + m_bounds.width - 8.0f;
        float trackY = m_bounds.y + 2.0f;
        float trackH = m_bounds.height - 4.0f;

        float contentH = itemH * m_itemDatas.size();
        float thumbH = std::max(20.0f, trackH * (trackH / contentH));
        float thumbY = trackY + (m_scrollY / m_maxScrollY) * (trackH - thumbH);

        Rect thumbRect(trackX, thumbY, 6.0f, thumbH);
        D2D1_COLOR_F thumbBg = m_isDraggingScrollbar
            ? D2D1::ColorF(0x99 / 255.0f, 0x99 / 255.0f, 0x99 / 255.0f, 0.8f)
            : D2D1::ColorF(0x55 / 255.0f, 0x55 / 255.0f, 0x55 / 255.0f, 0.5f);

        ctx.FillRoundedRect(thumbRect, 3.0f, thumbBg);
    }

    ctx.PopClip();
}

void ListBox::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    // Check scrollbar thumb click & drag
    if (m_maxScrollY > 0.0f) {
        float trackX = m_bounds.x + m_bounds.width - 10.0f;
        if (pt.x >= trackX) {
            m_isDraggingScrollbar = true;
            m_dragStartY = pt.y;
            m_dragStartScrollY = m_scrollY;
            return;
        }
    }

    int clickedIdx = GetItemIndexFromY(pt.y);
    if (clickedIdx >= 0) {
        SetSelectedIndex(clickedIdx);
    }
}

void ListBox::OnMouseDblClick(Point pt) {
    Control::OnMouseDblClick(pt);

    int clickedIdx = GetItemIndexFromY(pt.y);
    if (clickedIdx >= 0 && clickedIdx < static_cast<int>(m_itemDatas.size())) {
        m_onItemDoubleClickedEvent.Invoke(this, clickedIdx, m_itemDatas[clickedIdx].text);
    }
}

void ListBox::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);

    if (m_isDraggingScrollbar && m_isPressed) {
        float deltaY = pt.y - m_dragStartY;
        float trackH = m_bounds.height - 4.0f;
        float itemH = GetItemHeight();
        float contentH = itemH * m_itemDatas.size();
        float thumbH = std::max(20.0f, trackH * (trackH / contentH));
        float scrollableTrackH = trackH - thumbH;

        if (scrollableTrackH > 0.0f) {
            m_scrollY = m_dragStartScrollY + (deltaY / scrollableTrackH) * m_maxScrollY;
            ClampScroll();
        }
        return;
    }

    m_hoveredIndex = GetItemIndexFromY(pt.y);
}

void ListBox::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isDraggingScrollbar = false;
}

void ListBox::OnMouseWheel(float delta) {
    float scrollStep = GetItemHeight() * 3.0f; // Scroll 3 items per wheel tick
    m_scrollY -= delta * scrollStep;
    ClampScroll();
}

void ListBox::OnKeyDown(int vkCode) {
    if (m_itemDatas.empty()) return;

    int newIdx = m_selectedIndex;
    int visibleCount = static_cast<int>((m_bounds.height - 4.0f) / GetItemHeight());

    switch (vkCode) {
    case VK_UP:
        newIdx = std::max(0, m_selectedIndex - 1);
        break;
    case VK_DOWN:
        newIdx = std::min(static_cast<int>(m_itemDatas.size()) - 1, (m_selectedIndex == -1) ? 0 : m_selectedIndex + 1);
        break;
    case VK_PRIOR: // Page Up
        newIdx = std::max(0, m_selectedIndex - visibleCount);
        break;
    case VK_NEXT: // Page Down
        newIdx = std::min(static_cast<int>(m_itemDatas.size()) - 1, m_selectedIndex + visibleCount);
        break;
    case VK_HOME:
        newIdx = 0;
        break;
    case VK_END:
        newIdx = static_cast<int>(m_itemDatas.size()) - 1;
        break;
    }

    if (newIdx != m_selectedIndex) {
        SetSelectedIndex(newIdx);
    }
}

} // namespace CUI
