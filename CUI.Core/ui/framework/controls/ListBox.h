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
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_ARROW) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual void OnCharInput(wchar_t ch);
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnThemeChanged() override;

    struct ListBoxItemsProperty {
        ListBox* owner;
        ListBoxItemsProperty& operator=(const std::vector<std::string>& items) { owner->SetItems(items); return *this; }
        ListBoxItemsProperty& operator=(std::initializer_list<std::string> items) { owner->SetItems(std::vector<std::string>(items)); return *this; }
        ListBoxItemsProperty& operator=(const std::string& itemsCsv) { owner->SetItems(itemsCsv); return *this; }
        size_t size() const { return owner->GetItemCount(); }
    } Items{this};

    ReadOnlyProperty<size_t> RowCount{[this]() { return GetItemCount(); }};
    ReadOnlyProperty<size_t> ItemCount{[this]() { return GetItemCount(); }};

    struct ListBoxSelectionModeProperty {
        ListBox* owner;
        ListBoxSelectionModeProperty& operator=(ListBoxSelectionMode mode) { owner->SetSelectionMode(mode); return *this; }
        operator ListBoxSelectionMode() const { return owner->GetSelectionMode(); }
        ListBoxSelectionMode Get() const { return owner->GetSelectionMode(); }
    } SelectionMode{this};

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

    PropertyRef<int, PropertyId::SelectedIndex> SelectedIndex; // 选中项索引的响应式双向绑定属性代理
    /**
     * @brief 列表框多选选中的索引集合属性代理。
     */
    struct ListBoxSelectedIndicesProperty {
        ListBox* owner;
        operator const std::unordered_set<int>&() const { return owner->GetSelectedIndices(); }
        const std::unordered_set<int>& Get() const { return owner->GetSelectedIndices(); }
        size_t size() const { return owner->GetSelectedIndices().size(); }
    } SelectedIndices{this};

    /**
     * @brief 列表框当前选中的单项文本属性代理。
     */
    struct ListBoxSelectedItemProperty {
        ListBox* owner;
        ListBoxSelectedItemProperty& operator=(const std::string& item) { owner->SetSelectedItem(item); return *this; }
        operator std::string() const { return owner->GetSelectedItem(); }
        std::string Get() const { return owner->GetSelectedItem(); }
    } SelectedItem{this};

    /**
     * @brief 是否允许拖拽列表项属性代理。
     */
    struct ListBoxAllowDragProperty {
        ListBox* owner;
        ListBoxAllowDragProperty& operator=(bool d) { owner->SetAllowDrag(d); return *this; }
        operator bool() const { return owner->GetAllowDrag(); }
        bool Get() const { return owner->GetAllowDrag(); }
    } AllowDrag{this};

    /**
     * @brief 是否允许向列表框拖放放入项属性代理。
     */
    struct ListBoxAllowDropProperty {
        ListBox* owner;
        ListBoxAllowDropProperty& operator=(bool d) { owner->SetAllowDrop(d); return *this; }
        operator bool() const { return owner->GetAllowDrop(); }
        bool Get() const { return owner->GetAllowDrop(); }
    } AllowDrop{this};

    std::string GetSelectedItem() const;
    void SetSelectedItem(const std::string& item) {
        for (size_t i = 0; i < GetItemCount(); ++i) {
            if (GetItemAt(i) == item) {
                SetSelectedIndex(static_cast<int>(i));
                return;
            }
        }
        SetSelectedIndex(-1);
    }

    const std::unordered_set<int>& GetSelectedIndices() const { return m_selectedIndices; }
    void SetItemSelected(int index, bool selected);
    bool IsItemSelected(int index) const;
    void SelectAll();
    void ClearSelection();

    int GetCaretIndex() const { return m_caretIndex; }
    void SetCaretIndex(int index);

    // Virtual Mode (0 memory allocation for 100k/1M items)
    void SetVirtualCount(size_t count);
    void RefreshVirtualCount(size_t count);

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
