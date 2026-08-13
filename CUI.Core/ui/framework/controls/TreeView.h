#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Control.h"
#include "ChromiumScrollAnimator.h"
#include "ScrollbarAutoHide.h"
#include "../animation/AnimationSystem.h"
#include <memory>
#include <string>
#include <vector>

namespace CUI {

class TreeView;

struct TreeViewItem {
    std::string header;
    std::string icon;
    // Optional host payload (e.g. full filesystem path for FilePicker).
    std::string tag;
    // Optional native icon (non-owning). Prefer over text/emoji `icon` when set.
    HICON nativeIcon = nullptr;
    bool isExpanded = false;
    bool isSelected = false;
    TreeViewItem* parent = nullptr;
    std::vector<std::shared_ptr<TreeViewItem>> children;
    // 0.0 = fully collapsed, 1.0 = fully expanded.
    // Drives WinUI expansion height, opacity fading, and chevron rotation.
    AnimatedScalar expandAnim{ 0.0f };
};

class TreeView : public Control {
public:
    TreeView();
    virtual ~TreeView() = default;

    virtual const char* GetClassName() const override { return "TreeView"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual UIElement* HitTest(float x, float y) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseRightClick(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    bool OnAnimationTick() override;
    bool HasSelfAnimation() const override;

    void ClearItems();
    void AddItem(std::shared_ptr<TreeViewItem> item);
    std::shared_ptr<TreeViewItem> AddItem(const std::string& header, bool expanded = false);
    void SetItems(const std::vector<std::shared_ptr<TreeViewItem>>& items);

    const std::vector<std::shared_ptr<TreeViewItem>>& GetItems() const { return m_items; }
    std::shared_ptr<TreeViewItem> GetSelectedItem() const { return m_selectedItem; }
    void SetSelectedItem(std::shared_ptr<TreeViewItem> item);

    // Expand / collapse a branch (no-op if item has no children).
    void SetItemExpanded(std::shared_ptr<TreeViewItem> item, bool expanded);
    void ToggleExpanded(std::shared_ptr<TreeViewItem> item);

    float GetIndentWidth() const { return m_indentWidth; }
    void SetIndentWidth(float w) {
        m_indentWidth = w;
        MarkRenderContentDirty();
    }

    // Call after mutating TreeViewItem::children outside TreeView APIs
    // (e.g. lazy-load) so the flat visible list is rebuilt on next measure/render.
    void InvalidateVisibleItems() {
        m_visibleDirty = true;
        ClampScroll();
        MarkRenderContentDirty();
    }

    Event<TreeView*, std::shared_ptr<TreeViewItem>>& OnSelectionChanged() { return m_onSelectionChangedEvent; }
    Event<TreeView*, std::shared_ptr<TreeViewItem>>& OnItemToggled() { return m_onItemToggledEvent; }
    Event<TreeView*, std::shared_ptr<TreeViewItem>>& OnItemDoubleClicked() { return m_onItemDoubleClickedEvent; }

private:
    struct VisibleItem {
        std::shared_ptr<TreeViewItem> item;
        int depth = 0;
        float clipFactor = 1.0f; // Combined effective height scale from all ancestors
        float contentY = 0.0f;   // Top of row in content coordinates (for virtualization)
    };

    void RebuildVisibleItems() const;
    float ComputeContentHeight(
        const std::vector<std::shared_ptr<TreeViewItem>>& list,
        float parentClip) const;
    float GetTotalContentHeight() const;
    float GetViewportHeight() const;
    float GetMaxScroll() const;
    void ClampScroll();
    void StopSmoothScroll();
    bool AdvanceSmoothScroll();
    Rect GetScrollbarTrackRect() const;
    Rect GetScrollbarThumbRect() const;
    int GetVisibleIndexFromY(float y) const;
    int GetVisibleIndexOfItem(TreeViewItem* item) const;
    Rect GetItemRect(int visibleIndex) const;
    Rect GetToggleRect(const VisibleItem& visibleItem, const Rect& rowRect) const;
    Rect GetToggleHitRect(const VisibleItem& visibleItem, const Rect& rowRect) const;
    void ToggleItem(std::shared_ptr<TreeViewItem> item);
    void SetParentRecursive(const std::shared_ptr<TreeViewItem>& item, TreeViewItem* parent);
    std::shared_ptr<TreeViewItem> FindFirstVisibleSelectable(int startIndex, int direction) const;
    bool TickExpandAnims(const std::vector<std::shared_ptr<TreeViewItem>>& list, float dt);
    void StartSelectRipple(const std::shared_ptr<TreeViewItem>& item, Point pt);

    static constexpr float kScrollbarInset = 3.0f;
    static constexpr float kScrollbarWidth = 8.0f;

    float m_indentWidth = 18.0f;
    std::vector<std::shared_ptr<TreeViewItem>> m_items;
    std::shared_ptr<TreeViewItem> m_selectedItem;
    mutable std::vector<VisibleItem> m_visibleItems;
    mutable bool m_visibleDirty = true;
    mutable float m_cachedContentHeight = 0.0f;
    float m_scrollY = 0.0f;
    ChromiumScrollAnimator m_scrollAnimator;
    ScrollbarAutoHide m_scrollbarAutoHide;
    bool m_isMouseDown = false;
    bool m_isDraggingThumb = false;
    bool m_scrollbarHovered = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;
    bool m_expandAnimActive = false;
    int m_hoveredVisibleIndex = -1;
    int m_pressedVisibleIndex = -1;
    Event<TreeView*, std::shared_ptr<TreeViewItem>> m_onSelectionChangedEvent;
    Event<TreeView*, std::shared_ptr<TreeViewItem>> m_onItemToggledEvent;
    Event<TreeView*, std::shared_ptr<TreeViewItem>> m_onItemDoubleClickedEvent;

    // Selection reveal ripple (same as ListView, ~2x Button speed).
    std::weak_ptr<TreeViewItem> m_selectRippleItem;
    float m_selectRippleLocalX = 0.0f;
    float m_selectRippleLocalY = 0.0f;
    float m_selectRippleRadius = 0.0f;
    bool m_selectRippleActive = false;
    bool m_selectRippleCovered = false;
};

} // namespace CUI
