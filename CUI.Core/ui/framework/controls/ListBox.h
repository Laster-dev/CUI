#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Control.h"
#include "ScrollbarAutoHide.h"
#include "../render/RenderLayer.h"
#include "../dnd/DragDropService.h"
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

class ListBox : public Control, public IDragSource, public IDropTarget {
public:
    ListBox();
    virtual ~ListBox() override;

    virtual const char* GetClassName() const override { return "ListBox"; }
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_ARROW) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnCharInput(wchar_t ch);
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnThemeChanged() override;

    void SetProperty(PropertyId id, const Value& val) override;

    // Items & Data Management
    void AddItem(const std::string& item);
    void AddItem(std::shared_ptr<UIElement> customElement);
    void InsertItem(int index, const std::string& item);
    void RemoveItem(int index);
    void SetItems(const std::vector<std::string>& items);
    void SetItems(const std::string& itemsCsv);
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

    void SetAllowDrag(bool allow) { m_allowDrag = allow; }
    bool GetAllowDrag() const { return m_allowDrag; }
    void SetAllowDrop(bool allow) { m_allowDrop = allow; }
    bool GetAllowDrop() const { return m_allowDrop; }

    DataPackage BeginDrag(Point pt) override;
    DragDropEffects AllowedEffects() const override;
    void OnDragCompleted(DragDropEffects effect, IDropTarget* target) override;
    DragDropEffects OnDragOver(Point pt, const DataPackage& data, DragDropEffects allowed) override;
    void OnDragLeave() override;
    bool OnDrop(Point pt, DataPackage& data, DragDropEffects effect) override;
    Rect DropHighlightRect() const override { return m_bounds; }

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
    void InvalidateItemsLayer();
    int GetDropInsertIndex(Point pt) const;
    void NormalizeSelection();
    void ClearDropInsert();
    bool CanCacheFullItems() const;
    float GetItemsContentHeight() const;
    Rect GetItemsViewportRect() const;
    void PaintItemsRange(GraphicsContext& ctx, int startIdx, int endIdx, float itemW, float scrollY);
    void RenderItemsLayer(GraphicsContext& ctx, float itemW);

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
    ScrollbarAutoHide m_scrollbarAutoHide;

    // Full-content bitmap when height fits — scroll is sourceRect blit (ScrollViewer-style).
    RenderLayer m_itemsLayer;
    bool m_itemsLayerCachesFull = false;
    static constexpr float kMaxFullContentCacheHeight = 4096.0f;

    Event<ListBox*, int, const std::string&> m_onSelectionChangedEvent;
    Event<ListBox*, int, const std::string&> m_onItemDoubleClickedEvent;

    bool m_allowDrag = false;
    bool m_allowDrop = false;
    int m_pressIndex = -1;
    Point m_pressPt{};
    int m_dragSourceIndex = -1;
    int m_dropInsertIndex = -1;
};

} // namespace CUI
