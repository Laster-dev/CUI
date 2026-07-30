#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Control.h"
#include <memory>
#include <string>
#include <vector>

namespace CUI {

class TreeView;

struct TreeViewItem {
    std::string header;
    std::string icon;
    bool isExpanded = false;
    bool isSelected = false;
    TreeViewItem* parent = nullptr;
    std::vector<std::shared_ptr<TreeViewItem>> children;
};

class TreeView : public Control {
public:
    TreeView();
    virtual ~TreeView() = default;

    virtual const char* GetClassName() const override { return "TreeView"; }
    virtual HCURSOR GetCursor() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual UIElement* HitTest(float x, float y) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual void OnKeyDown(int vkCode) override;

    void ClearItems();
    void AddItem(std::shared_ptr<TreeViewItem> item);
    std::shared_ptr<TreeViewItem> AddItem(const std::string& header, bool expanded = false);
    void SetItems(const std::vector<std::shared_ptr<TreeViewItem>>& items);

    const std::vector<std::shared_ptr<TreeViewItem>>& GetItems() const { return m_items; }
    std::shared_ptr<TreeViewItem> GetSelectedItem() const { return m_selectedItem; }
    void SetSelectedItem(std::shared_ptr<TreeViewItem> item);

    float GetItemHeight() const { return GetProperty("itemHeight").AsFloat(24.0f); }
    void SetItemHeight(float h) { SetProperty("itemHeight", Value(h)); }
    float GetIndentWidth() const { return GetProperty("indentWidth").AsFloat(18.0f); }
    void SetIndentWidth(float w) { SetProperty("indentWidth", Value(w)); }

    Event<TreeView*, std::shared_ptr<TreeViewItem>>& OnSelectionChanged() { return m_onSelectionChangedEvent; }
    Event<TreeView*, std::shared_ptr<TreeViewItem>>& OnItemToggled() { return m_onItemToggledEvent; }

private:
    struct VisibleItem {
        std::shared_ptr<TreeViewItem> item;
        int depth = 0;
    };

    void RebuildVisibleItems() const;
    void ClampScroll();
    int GetVisibleIndexFromY(float y) const;
    int GetVisibleIndexOfItem(TreeViewItem* item) const;
    Rect GetItemRect(int visibleIndex) const;
    Rect GetToggleRect(const VisibleItem& visibleItem, const Rect& rowRect) const;
    void ToggleItem(std::shared_ptr<TreeViewItem> item);
    void SetParentRecursive(const std::shared_ptr<TreeViewItem>& item, TreeViewItem* parent);
    std::shared_ptr<TreeViewItem> FindFirstVisibleSelectable(int startIndex, int direction) const;

    std::vector<std::shared_ptr<TreeViewItem>> m_items;
    std::shared_ptr<TreeViewItem> m_selectedItem;
    mutable std::vector<VisibleItem> m_visibleItems;
    mutable bool m_visibleDirty = true;
    float m_scrollY = 0.0f;
    bool m_isMouseDown = false;
    int m_hoveredVisibleIndex = -1;
    int m_pressedVisibleIndex = -1;
    Event<TreeView*, std::shared_ptr<TreeViewItem>> m_onSelectionChangedEvent;
    Event<TreeView*, std::shared_ptr<TreeViewItem>> m_onItemToggledEvent;
};

} // namespace CUI
