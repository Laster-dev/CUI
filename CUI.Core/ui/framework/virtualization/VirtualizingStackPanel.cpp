#include "VirtualizingStackPanel.h"
#include "../animation/AnimationService.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>

namespace CUI {

VirtualizingStackPanel::VirtualizingStackPanel() {
    this->ApplyBackgroundToken(ThemeTokenId::CardBackground);
    this->ApplyBorderToken(ThemeTokenId::CardBorder);
    this->ApplySelectedBackgroundToken(ThemeTokenId::SelectedBackground);
    this->ApplyBorderThickness(1.0f);
    this->ApplyCornerRadius(4.0f);
    this->ApplyWidth(320.0f);
    this->ApplyHeight(400.0f);

    SelectedIndex.Initialize(*this);
    ItemHeight.Initialize(*this);

    m_scrollAnimator.Reset(0.0f);
    m_lastScrollTick = std::chrono::steady_clock::now();
}

VirtualizingStackPanel::~VirtualizingStackPanel() {
    AnimationService::Instance().UnregisterElement(this);
    m_realizedItems.clear();
    m_recyclePool.Clear();
}

void VirtualizingStackPanel::ApplyItemProvider(std::shared_ptr<IVirtualizingItemProvider> provider) {
    // 回收既有元素
    if (m_provider) {
        for (auto& [idx, element] : m_realizedItems) {
            if (element) {
                m_provider->RecycleItem(idx, element, m_recyclePool);
            }
        }
    }
    m_realizedItems.clear();
    this->ClearChildren();

    m_provider = std::move(provider);
    m_scrollOffsetY = 0.0f;
    m_scrollAnimator.Reset(0.0f);
    m_selectedIndex = -1;

    InvalidateMeasure();
    MarkRenderContentDirty();
}

void VirtualizingStackPanel::ApplyItemCount(size_t count) {
    if (!m_provider) {
        m_provider = std::make_shared<FunctionalVirtualizingItemProvider>(
            [count]() { return count; },
            m_itemHeight,
            [](size_t idx, RecyclePool& /*pool*/) -> std::shared_ptr<UIElement> {
                auto label = std::make_shared<UIElement>();
                label->ApplyText("Item #" + std::to_string(idx));
                return label;
            }
        );
    }
    InvalidateMeasure();
    MarkRenderContentDirty();
}

size_t VirtualizingStackPanel::GetItemCount() const {
    return m_provider ? m_provider->GetItemCount() : 0;
}

void VirtualizingStackPanel::ApplyItemHeight(float height) {
    if (height > 0.0f && std::abs(m_itemHeight - height) > 0.001f) {
        m_itemHeight = height;
        InvalidateMeasure();
        MarkRenderContentDirty();
    }
}

void VirtualizingStackPanel::ApplyBufferCount(int count) {
    m_bufferCount = (count < 1) ? 1 : count;
    InvalidateMeasure();
    MarkRenderContentDirty();
}

float VirtualizingStackPanel::GetTotalContentHeight() const {
    const size_t totalItems = GetItemCount();
    return static_cast<float>(totalItems) * m_itemHeight;
}

float VirtualizingStackPanel::GetMaxScroll() const {
    const float totalH = GetTotalContentHeight();
    const float viewH = m_bounds.height > 0.0f ? m_bounds.height : 400.0f;
    return (std::max)(0.0f, totalH - viewH);
}

void VirtualizingStackPanel::ClampScrollOffset() {
    const float maxScroll = GetMaxScroll();
    m_scrollOffsetY = (std::clamp)(m_scrollOffsetY, 0.0f, maxScroll);
}

void VirtualizingStackPanel::ApplyScrollOffsetY(float offset) {
    m_scrollOffsetY = offset;
    ClampScrollOffset();
    m_scrollAnimator.Reset(m_scrollOffsetY);
    UpdateVirtualization();
    MarkRenderContentDirty();
}

void VirtualizingStackPanel::ScrollTo(float offset) {
    m_scrollAnimator.ScrollBy(offset - m_scrollOffsetY, 0.0f, GetMaxScroll());
    AnimationService::Instance().RegisterElement(this);
}

void VirtualizingStackPanel::ScrollToIndex(size_t index) {
    const size_t total = GetItemCount();
    if (total == 0 || index >= total) return;

    const float targetY = static_cast<float>(index) * m_itemHeight;
    const float viewH = m_bounds.height > 0.0f ? m_bounds.height : 400.0f;

    if (targetY < m_scrollOffsetY) {
        ScrollTo(targetY);
    } else if (targetY + m_itemHeight > m_scrollOffsetY + viewH) {
        ScrollTo(targetY + m_itemHeight - viewH);
    }
}

void VirtualizingStackPanel::ApplySelectedIndex(int index) {
    if (m_selectedIndex != index) {
        m_selectedIndex = index;
        m_onSelectionChangedEvent.Invoke(this, static_cast<size_t>(index));
        MarkRenderContentDirty();
    }
}

void VirtualizingStackPanel::UpdateVirtualization() {
    const size_t totalItems = GetItemCount();
    if (!m_provider || totalItems == 0) {
        for (auto& [idx, element] : m_realizedItems) {
            if (element && m_provider) {
                m_provider->RecycleItem(idx, element, m_recyclePool);
            }
        }
        m_realizedItems.clear();
        this->ClearChildren();
        return;
    }

    const float viewH = m_bounds.height > 0.0f ? m_bounds.height : 400.0f;
    const float itemH = m_itemHeight > 0.0f ? m_itemHeight : 32.0f;

    // 计算当前可见的首尾索引
    int rawFirst = static_cast<int>(std::floor(m_scrollOffsetY / itemH)) - m_bufferCount;
    int rawLast = static_cast<int>(std::ceil((m_scrollOffsetY + viewH) / itemH)) + m_bufferCount;

    size_t firstVisible = static_cast<size_t>((std::max)(0, rawFirst));
    size_t lastVisible = static_cast<size_t>((std::min)(static_cast<int>(totalItems) - 1, (std::max)(0, rawLast)));

    m_firstVisibleIndex = firstVisible;
    m_lastVisibleIndex = lastVisible;

    // 1. 回收超出可见缓冲区的项
    std::vector<size_t> toRecycle;
    for (const auto& [idx, element] : m_realizedItems) {
        if (idx < firstVisible || idx > lastVisible) {
            toRecycle.push_back(idx);
        }
    }

    for (size_t idx : toRecycle) {
        auto it = m_realizedItems.find(idx);
        if (it != m_realizedItems.end()) {
            auto element = it->second;
            this->RemoveChildQuiet(element);
            m_provider->RecycleItem(idx, element, m_recyclePool);
            m_realizedItems.erase(it);
        }
    }

    // 2. 实体化新进入可视区域的项
    const float contentWidth = (std::max)(0.0f, m_bounds.width - 12.0f);
    for (size_t i = firstVisible; i <= lastVisible; ++i) {
        if (m_realizedItems.find(i) == m_realizedItems.end()) {
            auto element = m_provider->RealizeItem(i, m_recyclePool);
            if (element) {
                this->AddChildQuiet(element);
                element->Measure(Size(contentWidth, itemH));
                const float y = static_cast<float>(i) * itemH - m_scrollOffsetY;
                element->Arrange(Rect(m_bounds.x + 4.0f, m_bounds.y + y, contentWidth, itemH));
                m_realizedItems[i] = std::move(element);
            }
        }
    }
}

Size VirtualizingStackPanel::Measure(Size availableSize) {
    const float desiredW = (GetWidth() > 0.0f) ? GetWidth() : (availableSize.width > 0.0f ? availableSize.width : 320.0f);
    const float desiredH = (GetHeight() > 0.0f) ? GetHeight() : (availableSize.height > 0.0f ? availableSize.height : 400.0f);

    UpdateVirtualization();

    const float contentWidth = (std::max)(0.0f, desiredW - 12.0f);
    for (auto& [_, element] : m_realizedItems) {
        if (element) {
            element->Measure(Size(contentWidth, m_itemHeight));
        }
    }

    return Size(desiredW, desiredH);
}

void VirtualizingStackPanel::Arrange(Rect finalRect) {
    ApplyBounds(finalRect);
    ClampScrollOffset();
    UpdateVirtualization();

    const float contentWidth = (std::max)(0.0f, finalRect.width - 12.0f);
    for (auto& [idx, element] : m_realizedItems) {
        if (element) {
            const float y = static_cast<float>(idx) * m_itemHeight - m_scrollOffsetY;
            element->Arrange(Rect(finalRect.x + 4.0f, finalRect.y + y, contentWidth, m_itemHeight));
        }
    }
}

UIElement* VirtualizingStackPanel::HitTest(float x, float y) {
    if (!GetBounds().Contains(Point(x, y))) {
        return nullptr;
    }
    // 优先命中内部子项
    for (auto& [_, element] : m_realizedItems) {
        if (element && element->GetBounds().Contains(Point(x, y))) {
            UIElement* hit = element->HitTest(x, y);
            if (hit) return hit;
        }
    }
    return this;
}

void VirtualizingStackPanel::OnRender(GraphicsContext& ctx) {
    const Rect bounds = GetBounds();
    if (bounds.IsEmpty()) return;

    // 1. 绘制背景与边框
    const Color bg = ResolveThemeColor(ThemeTokenId::CardBackground, ThemeTokenId::WindowBackground);
    ctx.FillRoundedRect(bounds, GetCornerRadius(), bg);

    const Color border = ResolveThemeColor(ThemeTokenId::CardBorder, ThemeTokenId::InputBorder);
    ctx.DrawRoundedRect(bounds, GetCornerRadius(), border, GetBorderThickness());

    // 2. 局部裁剪绘制子元素
    ctx.PushClip(bounds);

    // 绘制高亮选中底色
    if (m_selectedIndex >= 0 && m_selectedIndex >= static_cast<int>(m_firstVisibleIndex) && m_selectedIndex <= static_cast<int>(m_lastVisibleIndex)) {
        const float selY = bounds.y + static_cast<float>(m_selectedIndex) * m_itemHeight - m_scrollOffsetY;
        const Rect selRect(bounds.x + 2.0f, selY, bounds.width - 12.0f, m_itemHeight);
        const Color selBg = ResolveThemeColor(ThemeTokenId::SelectedBackground, ThemeTokenId::AccentColor);
        ctx.FillRoundedRect(selRect, 4.0f, selBg);
    }

    // 绘制当前所有实体化的条目
    for (auto& [_, element] : m_realizedItems) {
        if (element) {
            element->Render(ctx);
        }
    }

    ctx.PopClip();

    // 3. 绘制平滑滚动条
    const float maxScroll = GetMaxScroll();
    if (maxScroll > 0.0f && m_scrollbarAutoHide.IsDrawn()) {
        const Rect track = GetScrollbarTrackRect();
        const Rect thumb = GetScrollbarThumbRect();

        const float opacity = m_scrollbarAutoHide.Opacity();
        const Color trackColor(0.0f, 0.0f, 0.0f, 0.05f * opacity);
        ctx.FillRoundedRect(track, 3.0f, trackColor);

        Color thumbColor = ResolveThemeColor(ThemeTokenId::TextSecondary, ThemeTokenId::CardBorder);
        thumbColor.a *= opacity * 0.7f;
        ctx.FillRoundedRect(thumb, 3.0f, thumbColor);
    }
}

void VirtualizingStackPanel::Render(GraphicsContext& ctx) {
    OnRender(ctx);
}

Rect VirtualizingStackPanel::GetScrollbarTrackRect() const {
    const Rect bounds = GetBounds();
    constexpr float kTrackWidth = 6.0f;
    return Rect(bounds.Right() - kTrackWidth - 2.0f, bounds.y + 4.0f, kTrackWidth, (std::max)(0.0f, bounds.height - 8.0f));
}

Rect VirtualizingStackPanel::GetScrollbarThumbRect() const {
    const Rect track = GetScrollbarTrackRect();
    const float totalH = GetTotalContentHeight();
    const float viewH = m_bounds.height > 0.0f ? m_bounds.height : 400.0f;
    if (totalH <= 0.0f || totalH <= viewH) return Rect();

    const float thumbH = (std::max)(20.0f, track.height * (viewH / totalH));
    const float availableTrack = track.height - thumbH;
    const float ratio = m_scrollOffsetY / (totalH - viewH);
    const float thumbY = track.y + availableTrack * (std::clamp)(ratio, 0.0f, 1.0f);

    return Rect(track.x, thumbY, track.width, thumbH);
}

void VirtualizingStackPanel::OnMouseDown(Point pt) {
    m_scrollbarAutoHide.NotifyActivity(this);

    // 检查是否点击滚动条
    const Rect thumb = GetScrollbarThumbRect();
    const Rect track = GetScrollbarTrackRect();

    if (thumb.Contains(pt)) {
        m_isDraggingScrollbar = true;
        m_dragStartMouseY = pt.y;
        m_dragStartScrollOffset = m_scrollOffsetY;
        m_scrollbarAutoHide.ApplyDragging(true, this);
        return;
    } else if (track.Contains(pt)) {
        if (pt.y < thumb.y) {
            ScrollTo(m_scrollOffsetY - m_bounds.height * 0.8f);
        } else if (pt.y > thumb.Bottom()) {
            ScrollTo(m_scrollOffsetY + m_bounds.height * 0.8f);
        }
        return;
    }

    // 点击列表条目
    const float relY = pt.y - m_bounds.y + m_scrollOffsetY;
    if (relY >= 0.0f && m_itemHeight > 0.0f) {
        size_t idx = static_cast<size_t>(relY / m_itemHeight);
        if (idx < GetItemCount()) {
            ApplySelectedIndex(static_cast<int>(idx));
            m_onItemClickedEvent.Invoke(this, idx);
        }
    }
}

void VirtualizingStackPanel::OnMouseMove(Point pt) {
    if (m_isDraggingScrollbar) {
        m_scrollbarAutoHide.NotifyActivity(this);
        const float deltaY = pt.y - m_dragStartMouseY;
        const Rect track = GetScrollbarTrackRect();
        const Rect thumb = GetScrollbarThumbRect();
        const float availableTrack = track.height - thumb.height;

        if (availableTrack > 0.0f) {
            const float maxScroll = GetMaxScroll();
            const float newOffset = m_dragStartScrollOffset + (deltaY / availableTrack) * maxScroll;
            ApplyScrollOffsetY(newOffset);
        }
        return;
    }

    if (GetScrollbarTrackRect().Contains(pt)) {
        m_scrollbarAutoHide.ApplyPointerOver(true, this);
    } else {
        m_scrollbarAutoHide.ApplyPointerOver(false, this);
    }
}

void VirtualizingStackPanel::OnMouseUp(Point /*pt*/) {
    if (m_isDraggingScrollbar) {
        m_isDraggingScrollbar = false;
        m_scrollbarAutoHide.ApplyDragging(false, this);
    }
}

void VirtualizingStackPanel::OnMouseLeave() {
    m_scrollbarAutoHide.ApplyPointerOver(false, this);
}

void VirtualizingStackPanel::OnMouseWheel(float delta) {
    m_scrollbarAutoHide.NotifyActivity(this);
    // 每次滚轮滚动 3 行高度
    const float scrollStep = -delta * (m_itemHeight * 3.0f / 120.0f);
    m_scrollAnimator.ScrollBy(scrollStep, 0.0f, GetMaxScroll());
    AnimationService::Instance().RegisterElement(this);
}

bool VirtualizingStackPanel::OnKeyDown(int vkCode) {
    const size_t total = GetItemCount();
    if (total == 0) return false;

    // VK_UP (38), VK_DOWN (40), VK_PRIOR (33), VK_NEXT (34), VK_HOME (36), VK_END (35)
    if (vkCode == 38) { // Up
        int next = (m_selectedIndex > 0) ? m_selectedIndex - 1 : 0;
        ApplySelectedIndex(next);
        ScrollToIndex(static_cast<size_t>(next));
        return true;
    } else if (vkCode == 40) { // Down
        int next = (m_selectedIndex < static_cast<int>(total) - 1) ? m_selectedIndex + 1 : static_cast<int>(total) - 1;
        ApplySelectedIndex(next);
        ScrollToIndex(static_cast<size_t>(next));
        return true;
    } else if (vkCode == 33) { // PageUp
        const int pageItems = static_cast<int>(m_bounds.height / m_itemHeight);
        int next = (std::max)(0, m_selectedIndex - pageItems);
        ApplySelectedIndex(next);
        ScrollToIndex(static_cast<size_t>(next));
        return true;
    } else if (vkCode == 34) { // PageDown
        const int pageItems = static_cast<int>(m_bounds.height / m_itemHeight);
        int next = (std::min)(static_cast<int>(total) - 1, m_selectedIndex + pageItems);
        ApplySelectedIndex(next);
        ScrollToIndex(static_cast<size_t>(next));
        return true;
    } else if (vkCode == 36) { // Home
        ApplySelectedIndex(0);
        ScrollToIndex(0);
        return true;
    } else if (vkCode == 35) { // End
        ApplySelectedIndex(static_cast<int>(total) - 1);
        ScrollToIndex(total - 1);
        return true;
    }
    return false;
}

bool VirtualizingStackPanel::OnAnimationTick() {
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - m_lastScrollTick).count();
    m_lastScrollTick = now;

    bool stillAnimating = false;
    if (m_scrollAnimator.IsActive()) {
        m_scrollAnimator.Tick(dt, 0.0f, GetMaxScroll());
        m_scrollOffsetY = m_scrollAnimator.Current();
        UpdateVirtualization();
        MarkRenderContentDirty();
        stillAnimating = true;
    }

    if (m_scrollbarAutoHide.Tick(static_cast<float>(dt))) {
        MarkRenderContentDirty();
        stillAnimating = true;
    }

    return stillAnimating;
}

bool VirtualizingStackPanel::HasSelfAnimation() const {
    return m_scrollAnimator.IsActive() || m_scrollbarAutoHide.IsDrawn();
}

void VirtualizingStackPanel::OnThemeChanged() {
    MarkRenderContentDirty();
}

} // namespace CUI
