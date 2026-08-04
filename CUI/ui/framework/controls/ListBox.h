#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Control.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <chrono>

namespace CUI {

enum class ListBoxSelectionMode {
    Single,
    Multiple,
    Extended
};

class ListBox : public Control {
public:
    ListBox();
    virtual ~ListBox() = default;

    virtual const char* GetClassName() const override { return "ListBox"; }
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_ARROW) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnCharInput(wchar_t ch);
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    // Items & Data Management
    void AddItem(const std::string& item);
    void AddItem(std::shared_ptr<UIElement> customElement);
    void SetItems(const std::vector<std::string>& items);
    void ClearItems();

    virtual UIElement* HitTest(float x, float y) override;

    size_t GetItemCount() const;
    std::string GetItemAt(size_t index) const;

    // Selection Management
    ListBoxSelectionMode GetSelectionMode() const { return m_selectionMode; }
    void SetSelectionMode(ListBoxSelectionMode mode) { m_selectionMode = mode; }

    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int index);
    std::string GetSelectedItem() const;

    const std::unordered_set<int>& GetSelectedIndices() const { return m_selectedIndices; }
    void SetItemSelected(int index, bool selected);
    bool IsItemSelected(int index) const;
    void SelectAll();
    void ClearSelection();

    int GetCaretIndex() const { return m_caretIndex; }
    void SetCaretIndex(int index);

    // Item height & Virtualization
    float GetItemHeight() const { return GetProperty("itemHeight").AsFloat(28.0f); }
    void SetItemHeight(float h) { SetProperty("itemHeight", Value(h)); }

    // Virtual Mode (0 memory allocation for 100k/1M items)
    void SetVirtualCount(size_t count);

    struct ListBoxDataSource {
        virtual ~ListBoxDataSource() = default;
        virtual std::string GetItemText(size_t index) = 0;
    };
    void SetVirtualMode(size_t count, ListBoxDataSource* dataSource);
    bool IsVirtualMode() const { return m_virtualMode; }

    // Events
    Event<ListBox*, int, const std::string&>& OnSelectionChanged() { return m_onSelectionChangedEvent; }
    Event<ListBox*, int, const std::string&>& OnItemDoubleClicked() { return m_onItemDoubleClickedEvent; }

    struct ListBoxItemData {
        std::string text;
        std::shared_ptr<UIElement> customElement = nullptr;
    };

private:
    void ClampScroll();
    int GetItemIndexFromY(float y) const;
    void EnsureVisible(int index);
    void SelectRange(int fromIdx, int toIdx, bool keepExisting = false);
    void PerformTypeSearch(wchar_t ch);

    std::vector<ListBoxItemData> m_itemDatas;
    bool m_virtualMode = false;
    size_t m_virtualCount = 0;
    ListBoxDataSource* m_dataSource = nullptr;

    ListBoxSelectionMode m_selectionMode = ListBoxSelectionMode::Single;
    int m_selectedIndex = -1;
    int m_caretIndex = -1;
    int m_anchorIndex = -1;
    int m_hoveredIndex = -1;
    std::unordered_set<int> m_selectedIndices;

    // Type-search state
    std::string m_searchBuffer;
    std::chrono::steady_clock::time_point m_lastSearchTime{};

    // Scrollbar state & Virtualization
    float m_scrollY = 0.0f;
    float m_targetScrollY = 0.0f;
    float m_maxScrollY = 0.0f;
    AnimatedScalar m_scrollYAnim{ 0.0f };
    bool m_isDraggingScrollbar = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;

    Event<ListBox*, int, const std::string&> m_onSelectionChangedEvent;
    Event<ListBox*, int, const std::string&> m_onItemDoubleClickedEvent;
};

} // namespace CUI
