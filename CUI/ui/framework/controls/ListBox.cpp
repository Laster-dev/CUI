#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ListBox.h"
#include "../style/ThemeManager.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <windows.h>

namespace CUI {

ListBox::ListBox() {
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
    SetFontFamily("微软雅黑");
    SetItemHeight(28.0f);
    SetCornerRadius(4.0f);
    SetWidth(240.0f);
    SetHeight(300.0f);
    m_itemsLayer.SetCacheable(true);
}

void ListBox::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::Items) { SetItems(val.AsString("")); return; }
    Control::SetProperty(id, val);
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

void ListBox::SetVirtualCount(size_t count) {
    m_virtualMode = true;
    m_virtualCount = count;
    m_itemDatas.clear();
    ClearChildren();
    ClearSelection();
    m_caretIndex = -1;
    m_anchorIndex = -1;
    ClampScroll();
}

void ListBox::SetVirtualMode(size_t count, ListBoxDataSource* dataSource) {
    m_virtualMode = true;
    m_virtualCount = count;
    m_dataSource = dataSource;
    m_itemDatas.clear();
    ClearChildren();
    ClearSelection();
    m_caretIndex = -1;
    m_anchorIndex = -1;
    ClampScroll();
}

void ListBox::SetItems(const std::vector<std::string>& items) {
    m_virtualMode = false;
    m_virtualCount = 0;
    m_dataSource = nullptr;
    m_itemDatas.clear();
    ClearChildren();
    for (const auto& s : items) {
        m_itemDatas.push_back({ s, nullptr });
    }
    ClearSelection();
    m_caretIndex = -1;
    m_anchorIndex = -1;
    m_scrollY = 0.0f;
    m_targetScrollY = 0.0f;
    InvalidateItemsLayer();
}

void ListBox::SetItems(const std::string& itemsCsv) {
    std::vector<std::string> items;
    std::stringstream ss(itemsCsv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) items.push_back(item);
    }
    SetItems(items);
}

void ListBox::ClearItems() {
    m_virtualMode = false;
    m_virtualCount = 0;
    m_dataSource = nullptr;
    m_itemDatas.clear();
    ClearChildren();
    ClearSelection();
    m_caretIndex = -1;
    m_anchorIndex = -1;
    m_scrollY = 0.0f;
    m_targetScrollY = 0.0f;
    InvalidateItemsLayer();
}

size_t ListBox::GetItemCount() const {
    return m_virtualMode ? m_virtualCount : m_itemDatas.size();
}

std::string ListBox::GetItemAt(size_t index) const {
    if (m_virtualMode) {
        if (m_dataSource && index < m_virtualCount) {
            return m_dataSource->GetItemText(index);
        }
        if (index < m_virtualCount) {
            return "Virtual Item #" + std::to_string(index + 1);
        }
        return "";
    }
    if (index < m_itemDatas.size()) {
        return m_itemDatas[index].text;
    }
    return "";
}

bool ListBox::IsItemSelected(int index) const {
    if (index < 0 || static_cast<size_t>(index) >= GetItemCount()) return false;
    return m_selectedIndices.find(index) != m_selectedIndices.end();
}

void ListBox::SetSelectedIndex(int index) {
    size_t count = GetItemCount();
    if (index < -1 || index >= static_cast<int>(count)) return;

    m_selectedIndices.clear();
    if (index >= 0) {
        m_selectedIndices.insert(index);
        m_selectedIndex = index;
        m_caretIndex = index;
        m_anchorIndex = index;
        EnsureVisible(index);
    } else {
        m_selectedIndex = -1;
    }
    m_onSelectionChangedEvent.Invoke(this, m_selectedIndex, GetSelectedItem());
    InvalidateItemsLayer();
}

void ListBox::SetCaretIndex(int index) {
    size_t count = GetItemCount();
    if (index >= 0 && index < static_cast<int>(count)) {
        m_caretIndex = index;
        EnsureVisible(index);
        InvalidateItemsLayer();
    }
}

void ListBox::SetItemSelected(int index, bool selected) {
    size_t count = GetItemCount();
    if (index < 0 || index >= static_cast<int>(count)) return;

    if (selected) {
        if (m_selectionMode == ListBoxSelectionMode::Single) {
            SetSelectedIndex(index);
            return;
        }
        m_selectedIndices.insert(index);
        m_selectedIndex = index;
    } else {
        m_selectedIndices.erase(index);
        if (m_selectedIndex == index) {
            m_selectedIndex = m_selectedIndices.empty() ? -1 : *m_selectedIndices.begin();
        }
    }
    m_onSelectionChangedEvent.Invoke(this, m_selectedIndex, GetSelectedItem());
    InvalidateItemsLayer();
}

void ListBox::SelectAll() {
    if (m_selectionMode == ListBoxSelectionMode::Single) return;
    m_selectedIndices.clear();
    size_t count = GetItemCount();
    for (size_t i = 0; i < count; ++i) {
        m_selectedIndices.insert(static_cast<int>(i));
    }
    if (count > 0) {
        m_selectedIndex = 0;
    }
    m_onSelectionChangedEvent.Invoke(this, m_selectedIndex, GetSelectedItem());
    InvalidateItemsLayer();
}

void ListBox::ClearSelection() {
    m_selectedIndices.clear();
    m_selectedIndex = -1;
    m_onSelectionChangedEvent.Invoke(this, -1, "");
    InvalidateItemsLayer();
}

void ListBox::SelectRange(int fromIdx, int toIdx, bool keepExisting) {
    size_t count = GetItemCount();
    if (count == 0) return;

    fromIdx = std::clamp(fromIdx, 0, static_cast<int>(count) - 1);
    toIdx = std::clamp(toIdx, 0, static_cast<int>(count) - 1);

    if (!keepExisting) {
        m_selectedIndices.clear();
    }

    int start = (std::min)(fromIdx, toIdx);
    int end = (std::max)(fromIdx, toIdx);

    for (int i = start; i <= end; ++i) {
        m_selectedIndices.insert(i);
    }
    m_selectedIndex = toIdx;
    m_caretIndex = toIdx;
    EnsureVisible(toIdx);
    m_onSelectionChangedEvent.Invoke(this, m_selectedIndex, GetSelectedItem());
    InvalidateItemsLayer();
}

std::string ListBox::GetSelectedItem() const {
    if (m_selectedIndex >= 0 && static_cast<size_t>(m_selectedIndex) < GetItemCount()) {
        return GetItemAt(m_selectedIndex);
    }
    return "";
}

UIElement* ListBox::HitTest(float x, float y) {
    if (GetVisibility() != Visibility::Visible) return nullptr;

    if (m_bounds.Contains(x, y)) {
        int idx = GetItemIndexFromY(y);
        size_t count = GetItemCount();
        if (idx >= 0 && idx < static_cast<int>(count)) {
            if (!m_virtualMode && m_itemDatas[idx].customElement) {
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
    float expW = GetWidth(); if (expW < 0) expW = 240.0f;
    float expH = GetHeight(); if (expH < 0) expH = 300.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ListBox::ClampScroll() {
    float itemH = GetItemHeight();
    size_t count = GetItemCount();
    float contentH = itemH * count;
    float viewH = (std::max)(0.0f, m_bounds.height - 4.0f);

    m_maxScrollY = (std::max)(0.0f, contentH - viewH);
    m_targetScrollY = std::clamp(m_targetScrollY, 0.0f, m_maxScrollY);
    m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScrollY);
    if (!UIElement::AreAnimationsEnabled() || m_maxScrollY == 0.0f) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
    }
}

int ListBox::GetItemIndexFromY(float y) const {
    float relativeY = y - m_bounds.y - 2.0f + m_scrollY;
    float itemH = GetItemHeight();
    if (relativeY < 0.0f) return -1;
    int idx = static_cast<int>(relativeY / itemH);
    size_t count = GetItemCount();
    if (idx >= 0 && idx < static_cast<int>(count)) {
        return idx;
    }
    return -1;
}

void ListBox::EnsureVisible(int index) {
    size_t count = GetItemCount();
    if (index < 0 || index >= static_cast<int>(count)) return;

    float itemH = GetItemHeight();
    float itemTop = index * itemH;
    float itemBottom = itemTop + itemH;
    float viewH = m_bounds.height - 4.0f;

    if (itemTop < m_targetScrollY) {
        m_targetScrollY = itemTop;
    } else if (itemBottom > m_targetScrollY + viewH) {
        m_targetScrollY = itemBottom - viewH;
    }
    ClampScroll();
    m_scrollYAnim.SetTarget(m_targetScrollY);
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
    } else {
        RequestAnimationTicks();
    }
    MarkRenderRectDirty(m_bounds);
}

void ListBox::InvalidateItemsLayer() {
    m_itemsLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty);
    m_itemsLayerCachesFull = false;
    MarkRenderRectDirty(m_bounds);
}

bool ListBox::CanCacheFullItems() const {
    const float contentH = GetItemsContentHeight();
    return contentH > 0.0f && contentH <= kMaxFullContentCacheHeight;
}

void ListBox::OnThemeChanged() {
    UIElement::OnThemeChanged();
    InvalidateItemsLayer();
}

float ListBox::GetItemsContentHeight() const {
    return GetItemHeight() * static_cast<float>(GetItemCount());
}

Rect ListBox::GetItemsViewportRect() const {
    return Rect(m_bounds.x + 2.0f, m_bounds.y + 2.0f,
                (std::max)(0.0f, m_bounds.width - 4.0f),
                (std::max)(0.0f, m_bounds.height - 4.0f));
}

void ListBox::PaintItemsRange(GraphicsContext& ctx, int startIdx, int endIdx, float itemW, float scrollY) {
    float itemH = GetItemHeight();
    float fontH = GetFontSize();
    std::string font = GetFontFamily();
    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    D2D1_COLOR_F selectedBg = ResolveThemeColor(GetSelectedBackgroundToken(), ThemeTokenId::SelectedBackground);
    D2D1_COLOR_F hoverBg = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground);
    D2D1_COLOR_F focusBorderColor = ResolveThemeColor(GetBorderToken(), ThemeTokenId::AccentColor);
    bool isFocused = m_isFocused;

    for (int i = startIdx; i <= endIdx; ++i) {
        float itemY = m_bounds.y + 2.0f + i * itemH - scrollY;
        Rect itemRect(m_bounds.x + 2.0f, itemY, itemW, itemH);

        bool isSelected = IsItemSelected(i);
        bool isHovered = (i == m_hoveredIndex);
        bool isCaret = (i == m_caretIndex);

        if (isSelected) {
            ctx.FillRoundedRect(itemRect, 2.0f, selectedBg);
        } else if (isHovered && IsEnabled()) {
            ctx.FillRoundedRect(itemRect, 2.0f, hoverBg);
        }

        if (isCaret && isFocused) {
            ctx.DrawRoundedRect(itemRect.Inflate(-1.0f), 2.0f, focusBorderColor, 1.0f);
        }

        if (!m_virtualMode && static_cast<size_t>(i) < m_itemDatas.size() && m_itemDatas[i].customElement) {
            m_itemDatas[i].customElement->Measure(Size(itemRect.width, itemRect.height));
            m_itemDatas[i].customElement->Arrange(itemRect);
            m_itemDatas[i].customElement->Render(ctx);
        } else {
            std::string itemText = GetItemAt(i);
            D2D1_COLOR_F textClr = isSelected ? ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary) : textColor;
            Rect textRect(itemRect.x + 8.0f, itemRect.y, itemRect.width - 16.0f, itemRect.height);
            ctx.DrawText(itemText, textRect, textClr, font, fontH, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

void ListBox::RenderItemsLayer(GraphicsContext& ctx, float itemW) {
    const float contentH = GetItemsContentHeight();
    const Rect viewport = GetItemsViewportRect();
    const float viewH = (std::min)(viewport.height, contentH);
    if (contentH <= 0.0f || itemW <= 0.0f || viewH <= 0.0f) {
        return;
    }

    const Size cacheSize(itemW, contentH);
    const bool sizeChanged =
        std::abs(m_itemsLayer.GetCacheSurfaceSize().width - cacheSize.width) > 0.5f
        || std::abs(m_itemsLayer.GetCacheSurfaceSize().height - cacheSize.height) > 0.5f;
    const bool needsRerender = sizeChanged
        || !m_itemsLayerCachesFull
        || !m_itemsLayer.IsValid()
        || m_itemsLayer.NeedsContentRaster()
        || !m_itemsLayer.GetCacheBitmap();

    if (needsRerender) {
        Rect contentWorld(m_bounds.x + 2.0f, m_bounds.y + 2.0f, itemW, contentH);
        D2D1_COLOR_F clearBg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
        clearBg.a = 1.0f;
        if (ctx.PushLayerTarget(m_itemsLayer, cacheSize, contentWorld, clearBg)) {
            auto* d2d = ctx.GetD2DContext();
            D2D1_MATRIX_3X2_F oldTransform{};
            d2d->GetTransform(&oldTransform);
            d2d->SetTransform(D2D1::Matrix3x2F::Translation(-contentWorld.x, -contentWorld.y));
            PaintItemsRange(ctx, 0, static_cast<int>(GetItemCount()) - 1, itemW, 0.0f);
            d2d->SetTransform(oldTransform);
            ctx.PopLayerTarget(m_itemsLayer);
            m_itemsLayer.Validate();
            m_itemsLayerCachesFull = true;
        }
    }

    const float srcY = std::clamp(m_scrollY, 0.0f, (std::max)(0.0f, contentH - viewH));
    Rect sourceRect(0.0f, srcY, itemW, viewH);
    Rect dest(viewport.x, viewport.y, itemW, viewH);
    ctx.PushClip(viewport);
    ctx.DrawLayer(m_itemsLayer, dest, &sourceRect);
    ctx.PopClip();
    m_itemsLayer.SetTranslation(0.0f, -m_scrollY);
}

void ListBox::Render(GraphicsContext& ctx) {
    if (GetVisibility() != Visibility::Visible) return;

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
    ClampScroll();

    float radius = GetCornerRadius();
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    float borderThick = GetBorderThickness();

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, borderThick);

    float itemH = GetItemHeight();
    float sbWidth = (m_maxScrollY > 0.0f) ? 8.0f : 0.0f;
    float itemW = m_bounds.width - 4.0f - sbWidth;
    size_t count = GetItemCount();

    if (count > 0 && CanCacheFullItems()) {
        RenderItemsLayer(ctx, itemW);
    } else if (count > 0) {
        ctx.PushClip(Rect(m_bounds.x + 1, m_bounds.y + 1, m_bounds.width - 2, m_bounds.height - 2));
        int startIdx = std::max(0, static_cast<int>(m_scrollY / itemH));
        int endIdx = std::min(static_cast<int>(count) - 1, static_cast<int>((m_scrollY + m_bounds.height) / itemH));
        PaintItemsRange(ctx, startIdx, endIdx, itemW, m_scrollY);
        ctx.PopClip();
    }

    if (m_maxScrollY > 0.0f && m_scrollbarAutoHide.IsDrawn()) {
        float trackX = m_bounds.x + m_bounds.width - 8.0f;
        float trackY = m_bounds.y + 2.0f;
        float trackH = m_bounds.height - 4.0f;

        float contentH = itemH * count;
        float thumbH = std::max(20.0f, trackH * (trackH / contentH));
        float thumbY = trackY + (m_scrollY / m_maxScrollY) * (trackH - thumbH);

        Rect thumbRect(trackX, thumbY, 6.0f, thumbH);
        const float vis = m_scrollbarAutoHide.Opacity();
        D2D1_COLOR_F thumbBg = m_isDraggingScrollbar
            ? D2D1::ColorF(border.r, border.g, border.b, 0.8f * vis)
            : D2D1::ColorF(border.r, border.g, border.b, 0.5f * vis);

        ctx.FillRoundedRect(thumbRect, 3.0f, thumbBg);
    }
}

void ListBox::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    // Check scrollbar thumb click & drag
    if (m_maxScrollY > 0.0f) {
        float trackX = m_bounds.x + m_bounds.width - 10.0f;
        if (pt.x >= trackX) {
            m_isDraggingScrollbar = true;
            m_scrollbarAutoHide.SetDragging(true, this);
            m_scrollbarAutoHide.NotifyActivity(this);
            RequestAnimationTicks();
            m_dragStartY = pt.y;
            m_dragStartScrollY = m_scrollY;
            return;
        }
    }

    int clickedIdx = GetItemIndexFromY(pt.y);
    if (clickedIdx < 0 || static_cast<size_t>(clickedIdx) >= GetItemCount()) return;

    bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    if (m_selectionMode == ListBoxSelectionMode::Extended) {
        if (shiftDown) {
            int anchor = (m_anchorIndex >= 0) ? m_anchorIndex : clickedIdx;
            SelectRange(anchor, clickedIdx, ctrlDown);
            m_caretIndex = clickedIdx;
        } else if (ctrlDown) {
            bool currentSelected = IsItemSelected(clickedIdx);
            SetItemSelected(clickedIdx, !currentSelected);
            m_caretIndex = clickedIdx;
            m_anchorIndex = clickedIdx;
        } else {
            SetSelectedIndex(clickedIdx);
        }
    } else if (m_selectionMode == ListBoxSelectionMode::Multiple) {
        bool currentSelected = IsItemSelected(clickedIdx);
        SetItemSelected(clickedIdx, !currentSelected);
        m_caretIndex = clickedIdx;
    } else {
        SetSelectedIndex(clickedIdx);
    }
}

void ListBox::OnMouseDblClick(Point pt) {
    Control::OnMouseDblClick(pt);

    int clickedIdx = GetItemIndexFromY(pt.y);
    if (clickedIdx >= 0 && static_cast<size_t>(clickedIdx) < GetItemCount()) {
        m_onItemDoubleClickedEvent.Invoke(this, clickedIdx, GetItemAt(clickedIdx));
    }
}

void ListBox::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);

    const bool overBar = m_maxScrollY > 0.0f && pt.x >= m_bounds.x + m_bounds.width - 10.0f;
    m_scrollbarAutoHide.SetPointerOver(overBar, this);
    if (overBar) {
        RequestAnimationTicks();
    }

    if (m_isDraggingScrollbar && m_isPressed) {
        float deltaY = pt.y - m_dragStartY;
        float trackH = m_bounds.height - 4.0f;
        float itemH = GetItemHeight();
        size_t count = GetItemCount();
        float contentH = itemH * count;
        float thumbH = std::max(20.0f, trackH * (trackH / contentH));
        float scrollableTrackH = trackH - thumbH;

        if (scrollableTrackH > 0.0f && m_maxScrollY > 0.0f) {
            m_targetScrollY = m_dragStartScrollY + (deltaY / scrollableTrackH) * m_maxScrollY;
            ClampScroll();
            m_scrollY = m_targetScrollY;
            m_scrollYAnim.Reset(m_scrollY);
            m_scrollbarAutoHide.NotifyActivity(this);
            MarkRenderRectDirty(m_bounds);
        } else {
            m_scrollY = 0.0f;
            m_targetScrollY = 0.0f;
            m_scrollYAnim.Reset(0.0f);
        }
        return;
    }

    const int newHover = GetItemIndexFromY(pt.y);
    if (newHover != m_hoveredIndex) {
        m_hoveredIndex = newHover;
        InvalidateItemsLayer();
    }
}

void ListBox::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isDraggingScrollbar = false;
    m_scrollbarAutoHide.SetDragging(false, this);
    RequestAnimationTicks();
}

void ListBox::OnMouseLeave() {
    Control::OnMouseLeave();
    m_scrollbarAutoHide.SetPointerOver(false, this);
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        InvalidateItemsLayer();
    }
    RequestAnimationTicks();
}

void ListBox::OnMouseWheel(float delta) {
    if (m_maxScrollY <= 0.0f) {
        m_scrollY = 0.0f;
        m_targetScrollY = 0.0f;
        m_scrollYAnim.Reset(0.0f);
        // Bubble so parent page ScrollViewer can still scroll.
        UIElement::OnMouseWheel(delta);
        return;
    }

    const float prevTarget = m_targetScrollY;
    float scrollStep = GetItemHeight() * 2.5f;
    m_targetScrollY -= delta * scrollStep;
    ClampScroll();
    if (std::abs(m_targetScrollY - prevTarget) < 0.001f) {
        UIElement::OnMouseWheel(delta);
        return;
    }

    m_scrollYAnim.SetTarget(m_targetScrollY);
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
    } else {
        // Same as ScrollViewer: wheel must re-register for AnimationManager ticks.
        RequestAnimationTicks();
    }
    m_scrollbarAutoHide.NotifyActivity(this);
    MarkRenderRectDirty(m_bounds);
}

bool ListBox::OnAnimationTick() {
    // Skip Control hover visual-state pump — list chrome is not Control-fill driven.
    bool base = UIElement::OnAnimationTick();
    float dt = UIElement::GetAnimationDeltaSeconds();
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
        const bool hideAnimating = m_scrollbarAutoHide.Tick(dt);
        if (hideAnimating) {
            MarkRenderRectDirty(m_bounds);
        }
        return base || hideAnimating;
    }
    m_scrollYAnim.SetTarget(m_targetScrollY);
    const float prevScroll = m_scrollY;
    bool anim = m_scrollYAnim.Tick(dt, AnimationSpec{ 0.55f, 0.5f });
    if (anim) {
        m_scrollY = m_scrollYAnim.Current();
        m_scrollbarAutoHide.NotifyActivity(this);
        if (std::abs(m_scrollY - prevScroll) > 0.25f) {
            MarkRenderRectDirty(m_bounds);
        }
    }
    const float prevOpacity = m_scrollbarAutoHide.Opacity();
    const bool hideAnimating = m_scrollbarAutoHide.Tick(dt);
    if (std::abs(prevOpacity - m_scrollbarAutoHide.Opacity()) > 0.001f) {
        MarkRenderRectDirty(m_bounds);
    }
    if (anim || hideAnimating) {
        RequestAnimationTicks();
    }
    return base || anim || hideAnimating;
}

bool ListBox::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || std::abs(m_scrollYAnim.Target() - m_scrollYAnim.Current()) > 0.001f
        || m_scrollbarAutoHide.NeedsTicks();
}

void ListBox::OnKeyDown(int vkCode) {
    size_t count = GetItemCount();
    if (count == 0) return;

    bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    int newCaret = (m_caretIndex >= 0) ? m_caretIndex : 0;
    int visibleCount = static_cast<int>((m_bounds.height - 4.0f) / GetItemHeight());

    switch (vkCode) {
    case VK_UP:
        newCaret = std::max(0, newCaret - 1);
        break;
    case VK_DOWN:
        newCaret = std::min(static_cast<int>(count) - 1, newCaret + 1);
        break;
    case VK_PRIOR: // Page Up
        newCaret = std::max(0, newCaret - visibleCount);
        break;
    case VK_NEXT: // Page Down
        newCaret = std::min(static_cast<int>(count) - 1, newCaret + visibleCount);
        break;
    case VK_HOME:
        newCaret = 0;
        break;
    case VK_END:
        newCaret = static_cast<int>(count) - 1;
        break;
    case VK_SPACE:
        if (m_caretIndex >= 0) {
            if (m_selectionMode == ListBoxSelectionMode::Extended || m_selectionMode == ListBoxSelectionMode::Multiple) {
                SetItemSelected(m_caretIndex, !IsItemSelected(m_caretIndex));
            } else {
                SetSelectedIndex(m_caretIndex);
            }
        }
        return;
    case 'A':
        if (ctrlDown && m_selectionMode != ListBoxSelectionMode::Single) {
            SelectAll();
            return;
        }
        break;
    }

    if (newCaret != m_caretIndex) {
        m_caretIndex = newCaret;
        EnsureVisible(m_caretIndex);

        if (m_selectionMode == ListBoxSelectionMode::Extended) {
            if (shiftDown) {
                int anchor = (m_anchorIndex >= 0) ? m_anchorIndex : 0;
                SelectRange(anchor, m_caretIndex, ctrlDown);
            } else if (!ctrlDown) {
                SetSelectedIndex(m_caretIndex);
            }
        } else if (m_selectionMode == ListBoxSelectionMode::Single) {
            SetSelectedIndex(m_caretIndex);
        }
        InvalidateItemsLayer();
    }
}

void ListBox::OnCharInput(wchar_t ch) {
    if (ch >= L' ' && ch <= L'~') {
        PerformTypeSearch(ch);
    }
}

void ListBox::PerformTypeSearch(wchar_t ch) {
    using clock = std::chrono::steady_clock;
    auto now = clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastSearchTime).count();
    m_lastSearchTime = now;

    if (elapsedMs > 1000) {
        m_searchBuffer.clear();
    }
    m_searchBuffer += static_cast<char>(tolower(static_cast<unsigned char>(ch)));

    size_t count = GetItemCount();
    if (count == 0) return;

    int startIdx = (m_caretIndex >= 0) ? (m_caretIndex + 1) % static_cast<int>(count) : 0;
    for (size_t i = 0; i < count; ++i) {
        int idx = static_cast<int>((startIdx + i) % count);
        std::string itemText = GetItemAt(idx);
        std::string lowerText = itemText;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);

        if (lowerText.rfind(m_searchBuffer, 0) == 0) {
            SetSelectedIndex(idx);
            break;
        }
    }
}

} // namespace CUI
