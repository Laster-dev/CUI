#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Control.h"
#include <vector>
#include <string>

namespace CUI {

class ListBox : public Control {
public:
    ListBox();
    virtual ~ListBox() = default;

    virtual const char* GetClassName() const override { return "ListBox"; }
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_ARROW) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnMouseWheel(float delta) override;

    // Items & Data Management
    void AddItem(const std::string& item);
    void SetItems(const std::vector<std::string>& items);
    void ClearItems();

    size_t GetItemCount() const { return m_items.size(); }
    std::string GetItemAt(size_t index) const;

    // Selection
    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int index);
    std::string GetSelectedItem() const;

    // Item height & Virtualization
    float GetItemHeight() const { return GetProperty("itemHeight").AsFloat(28.0f); }
    void SetItemHeight(float h) { SetProperty("itemHeight", Value(h)); }

    // Events
    Event<ListBox*, int, const std::string&>& OnSelectionChanged() { return m_onSelectionChangedEvent; }
    Event<ListBox*, int, const std::string&>& OnItemDoubleClicked() { return m_onItemDoubleClickedEvent; }

private:
    void ClampScroll();
    int GetItemIndexFromY(float y) const;
    void EnsureVisible(int index);

    std::vector<std::string> m_items;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;

    // Scrollbar state & Virtualization
    float m_scrollY = 0.0f;
    float m_maxScrollY = 0.0f;
    bool m_isDraggingScrollbar = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;

    Event<ListBox*, int, const std::string&> m_onSelectionChangedEvent;
    Event<ListBox*, int, const std::string&> m_onItemDoubleClickedEvent;
};

} // namespace CUI
