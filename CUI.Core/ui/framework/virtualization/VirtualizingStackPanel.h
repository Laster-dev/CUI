#pragma once

#include "../controls/Control.h"
#include "../controls/ChromiumScrollAnimator.h"
#include "../controls/ScrollbarAutoHide.h"
#include "RecyclePool.h"
#include "VirtualizingItemProvider.h"
#include <chrono>
#include <memory>
#include <unordered_map>

namespace CUI {

/**
 * @brief 高性能虚拟化垂直堆叠面板（VirtualizingStackPanel）。
 * 遵循现代 UI 框架的虚拟化规范（WPF/WinUI 3/Flutter Sliver），
 * 仅对当前视口可见区域及邻近缓冲区的条目执行 Measure / Arrange / Render，
 * 配合 RecyclePool 实例复用池，支持 1,000,000+ 海量数据秒级渲染与丝滑 120 FPS 惯性滚动。
 */
class VirtualizingStackPanel : public Control {
public:
    VirtualizingStackPanel();
    virtual ~VirtualizingStackPanel() override;

    virtual const char* GetClassName() const override { return "VirtualizingStackPanel"; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual UIElement* HitTest(float x, float y) override;

    // 输入与交互回调
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnThemeChanged() override;

    // 数据提供者接口
    void ApplyItemProvider(std::shared_ptr<IVirtualizingItemProvider> provider);
    std::shared_ptr<IVirtualizingItemProvider> GetItemProvider() const { return m_provider; }

    // 便捷模板配置接口
    void ApplyItemCount(size_t count);
    size_t GetItemCount() const;

    void ApplyItemHeight(float height);
    float GetItemHeight() const { return m_itemHeight; }

    void ApplyBufferCount(int count);
    int GetBufferCount() const { return m_bufferCount; }

    // 滚动与视口
    float GetScrollOffsetY() const { return m_scrollOffsetY; }
    void ApplyScrollOffsetY(float offset);
    void ScrollTo(float offset);
    void ScrollToIndex(size_t index);

    // 选中管理
    int GetSelectedIndex() const { return m_selectedIndex; }
    void ApplySelectedIndex(int index);

    // 回收池存取
    RecyclePool& GetRecyclePool() { return m_recyclePool; }
    const RecyclePool& GetRecyclePool() const { return m_recyclePool; }

    // 诊断统计信息
    size_t GetRealizedChildCount() const { return m_realizedItems.size(); }
    size_t GetFirstVisibleIndex() const { return m_firstVisibleIndex; }
    size_t GetLastVisibleIndex() const { return m_lastVisibleIndex; }

    // 事件中心
    Event<VirtualizingStackPanel*, size_t>& OnItemClicked() { return m_onItemClickedEvent; }
    Event<VirtualizingStackPanel*, size_t>& OnSelectionChanged() { return m_onSelectionChangedEvent; }

    // 响应式属性代理
    PropertyRef<int, PropertyId::SelectedIndex> SelectedIndex;
    PropertyRef<float, PropertyId::ItemHeight> ItemHeight;

private:
    float GetMaxScroll() const;
    float GetTotalContentHeight() const;
    Rect GetScrollbarTrackRect() const;
    Rect GetScrollbarThumbRect() const;
    void ClampScrollOffset();
    void UpdateVirtualization();

    std::shared_ptr<IVirtualizingItemProvider> m_provider;
    RecyclePool m_recyclePool;

    // 当前视口中已实体化的子元素集合（项索引 -> 对应 UIElement 实例）
    std::unordered_map<size_t, std::shared_ptr<UIElement>> m_realizedItems;

    float m_itemHeight = 36.0f;
    int m_bufferCount = 3;
    float m_scrollOffsetY = 0.0f;
    int m_selectedIndex = -1;

    size_t m_firstVisibleIndex = 0;
    size_t m_lastVisibleIndex = 0;

    // 滚动与滚动条状态机
    ChromiumScrollAnimator m_scrollAnimator;
    ScrollbarAutoHide m_scrollbarAutoHide;
    std::chrono::steady_clock::time_point m_lastScrollTick;
    bool m_isDraggingScrollbar = false;
    float m_dragStartMouseY = 0.0f;
    float m_dragStartScrollOffset = 0.0f;

    Event<VirtualizingStackPanel*, size_t> m_onItemClickedEvent;
    Event<VirtualizingStackPanel*, size_t> m_onSelectionChangedEvent;
};

} // namespace CUI
