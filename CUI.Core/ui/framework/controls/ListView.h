#pragma once
#include "Control.h"
#include "ChromiumScrollAnimator.h"
#include "ScrollbarAutoHide.h"
#include "../render/RenderLayer.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <functional>
#include <chrono>

namespace CUI {

class ContextMenu;

enum class ListViewSelectionMode {
    Single,
    Multiple,
    Extended
};

struct ListViewColumn {
    std::string header;
    float width = 120.0f;
    float minWidth = 40.0f;
    bool isResizable = true;
    bool visible = true;
    bool sortable = true;
};

struct ListViewCellData {
    std::string text;
    std::shared_ptr<UIElement> customElement = nullptr;
};

// Virtual data source for high-performance 100k+ row ListView
class ListViewDataSource {
public:
    virtual ~ListViewDataSource() = default;
    virtual std::string GetCellText(int row, int col) = 0;
    virtual std::shared_ptr<UIElement> GetCellElement(int row, int col) { return nullptr; }
    // Optional small icon for column 0 (non-owning; caller retains lifetime).
    virtual HICON GetRowIcon(int row) { (void)row; return nullptr; }
};

class ListView : public Control {
public:
    ListView();
    virtual ~ListView() = default;

    virtual const char* GetClassName() const override { return "ListView"; }
    virtual HCURSOR GetCursor() const override;

    // 反射式属性存取（供 PropertyRef 绑定系统使用）
    virtual void SetProperty(PropertyId id, const Value& val) override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual UIElement* HitTest(float x, float y) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseRightClick(Point pt) override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual bool OnContextMenuRelease(Point pt) override;
    virtual bool OnMiddleButtonDown(Point pt) override;
    virtual void OnMiddleButtonUp(Point pt) override;
    virtual bool IsMiddleScrollActive() const override { return m_middleScrollActive; }
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual void OnMouseWheel(float delta) override;
    virtual void OnAutoScrollTick() override;
    virtual bool NeedsAutoScrollTick() const override { return m_isRubberBandSelecting || m_middleScrollActive; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnThemeChanged() override;

    /**
     * @brief 多列列表视图多选行索引集合属性代理。
     */
    struct ListViewSelectedIndicesProperty {
        ListView* owner;
        operator const std::unordered_set<int>&() const { return owner->GetSelectedIndices(); }
        const std::unordered_set<int>& Get() const { return owner->GetSelectedIndices(); }
        size_t size() const { return owner->GetSelectedIndices().size(); }
        bool empty() const { return owner->GetSelectedIndices().empty(); }
    } SelectedIndices{this};

    /**
     * @brief 列表视图当前活动光标行索引属性代理。
     */
    struct ListViewCaretIndexProperty {
        ListView* owner;
        ListViewCaretIndexProperty& operator=(int idx) { owner->SetCaretIndex(idx); return *this; }
        operator int() const { return owner->GetCaretIndex(); }
        int Get() const { return owner->GetCaretIndex(); }
    } CaretIndex{this};

    // Columns Management
    void AddColumn(const std::string& header, float width = 120.0f);
    void ClearColumns();
    const std::vector<ListViewColumn>& GetColumns() const { return m_columns; }
    void SetColumnVisible(int columnIndex, bool visible);
    bool IsColumnVisible(int columnIndex) const;
    int GetSortColumn() const { return m_sortColumn; }
    bool IsSortAscending() const { return m_sortAscending; }

    using ShellContextMenuHandler = std::function<bool(Point clientPt, const std::vector<int>& rows)>;
    void SetShellContextMenuHandler(ShellContextMenuHandler handler) { m_shellContextMenuHandler = std::move(handler); }

    struct ListViewRowsProperty {
        ListView* owner;
        ListViewRowsProperty& operator=(const std::vector<std::vector<std::string>>& rowsData) { owner->SetRows(rowsData); return *this; }
        ListViewRowsProperty& operator=(const std::vector<std::vector<ListViewCellData>>& rowsData) { owner->SetRows(rowsData); return *this; }
        size_t size() const { return owner->GetRowCount(); }
    } Rows{this};

    ReadOnlyProperty<size_t> RowCount{[this]() { return GetRowCount(); }};

    struct ListViewShowGridLinesProperty {
        ListView* owner;
        ListViewShowGridLinesProperty& operator=(bool show) { owner->SetShowGridLines(show); return *this; }
        operator bool() const { return owner->GetShowGridLines(); }
        bool Get() const { return owner->GetShowGridLines(); }
    } ShowGridLines{this};

    struct ListViewRowHeightProperty {
        ListView* owner;
        ListViewRowHeightProperty& operator=(float h) { owner->SetRowHeight(h); return *this; }
        operator float() const { return owner->GetRowHeight(); }
        float Get() const { return owner->GetRowHeight(); }
    } RowHeight{this};

    struct ListViewSelectionModeProperty {
        ListView* owner;
        ListViewSelectionModeProperty& operator=(ListViewSelectionMode mode) { owner->SetSelectionMode(mode); return *this; }
        operator ListViewSelectionMode() const { return owner->GetSelectionMode(); }
        ListViewSelectionMode Get() const { return owner->GetSelectionMode(); }
    } SelectionMode{this};

    // In-Memory Data Rows Management
    void AddRow(const std::vector<std::string>& rowData);
    void AddRow(const std::vector<ListViewCellData>& rowData);
    void SetRows(const std::vector<std::vector<std::string>>& rowsData);
    void SetRows(const std::vector<std::vector<ListViewCellData>>& rowsData);
    void ClearRows();
    size_t GetRowCount() const;

    // Optional per-row icons drawn in column 0 (non-owning HICONs).
    void SetRowIcons(const std::vector<HICON>& icons);
    void ClearRowIcons();

    // Virtual Mode
    void SetVirtualMode(int rowCount, ListViewDataSource* dataSource);
    void SetVirtualRowCount(int rowCount);
    bool IsVirtualMode() const { return m_virtualMode; }
    void RefreshRows();
    // Row Height
    float GetRowHeight() const { return m_rowHeight; }
    void SetRowHeight(float h) { m_rowHeight = h; }

    // Content area column/row separators (Everything-style lists usually hide these).
    void SetShowGridLines(bool show) { m_showGridLines = show; InvalidateRowsLayer(); }
    bool GetShowGridLines() const { return m_showGridLines; }

    // Selection Management
    ListViewSelectionMode GetSelectionMode() const { return m_selectionMode; }
    void SetSelectionMode(ListViewSelectionMode mode) { m_selectionMode = mode; }

    const std::unordered_set<int>& GetSelectedIndices() const { return m_selectedIndices; }
    void SelectAll();
    void ClearSelection();
    void SetRowSelected(int rowIndex, bool selected);
    bool IsRowSelected(int rowIndex) const;

    int GetCaretIndex() const { return m_caretIndex; }
    void SetCaretIndex(int index);
    void EnsureVisible(int rowIndex);

    PropertyRef<int, PropertyId::SelectedIndex> SelectedIndex; // 主选中行（caret 行）索引的响应式双向绑定属性代理

    // Events
    Event<ListView*, int>& OnSelectionChanged() { return m_onSelectionChangedEvent; }
    Event<ListView*, int>& OnRowDoubleClicked() { return m_onRowDoubleClickedEvent; }
    Event<ListView*, int, bool>& OnColumnHeaderClicked() { return m_onColumnSortEvent; }

private:
    int HitTestHeaderColumn(float x) const;
    void RebuildHeaderContextMenu();
    void SortIndicatorForColumn(int colIdx, std::string& headerOut) const;
    void ClampScroll();
    int GetRowIndexFromY(float y) const;
    int GetColumnIndexFromX(float x) const;
    void UpdateRubberBandSelection();
    float GetTotalColumnsWidth() const;
    float GetColumnWidth(size_t index) const;
    bool ApplyAutoScroll();
    void SelectRange(int fromIdx, int toIdx, bool keepExisting = false);
    void InvalidateRowsLayer();
    bool CanCacheFullRows() const;
    float GetRowsContentHeight() const;
    Rect GetRowsViewportRect() const;
    void PaintRowsRange(GraphicsContext& ctx, int startRow, int endRow, float scrollX, float scrollY);
    void RenderRowsLayer(GraphicsContext& ctx);
    void StartSelectRipple(int row, Point pt);
    Rect GetRowPillRect(int row, float scrollY) const;

    std::string GetCellText(int row, int col) const;
    std::shared_ptr<UIElement> GetCellElement(int row, int col) const;

    std::vector<ListViewColumn> m_columns;
    std::vector<std::vector<ListViewCellData>> m_rows;
    std::vector<HICON> m_rowIcons;

    // Virtual mode state
    bool m_virtualMode = false;
    int m_virtualRowCount = 0;
    ListViewDataSource* m_dataSource = nullptr;

    ListViewSelectionMode m_selectionMode = ListViewSelectionMode::Extended;
    std::unordered_set<int> m_selectedIndices;
    int m_anchorIndex = -1;
    int m_caretIndex = -1;
    int m_hoveredRowIndex = -1;
    int m_hoveredColumnSplitter = -1;
    int m_headerClickCol = -1;
    bool m_headerContextMenuPending = false;
    int m_sortColumn = -1;
    bool m_sortAscending = true;
    std::shared_ptr<ContextMenu> m_headerContextMenu;
    ShellContextMenuHandler m_shellContextMenuHandler;

    // Column resizing state
    bool m_isResizingColumn = false;
    int m_resizingColumnIndex = -1;
    float m_dragStartX = 0.0f;
    float m_initialColumnWidth = 0.0f;

    // Column reordering state
    bool m_isReorderingColumn = false;
    int m_reorderingColumnIndex = -1;
    float m_columnDragStartX = 0.0f;
    float m_columnDragCurrentX = 0.0f;

    // Rubber-band selection state
    bool m_isMouseDown = false;
    bool m_isRightMouseDown = false;
    bool m_rightDragDidRubberBand = false;
    Point m_mouseDownPoint;
    int m_pendingRowClick = -1;
    std::unordered_set<int> m_initialSelectedBeforeDrag;

    bool m_isRubberBandSelecting = false;
    Point m_rubberBandStart;     // content coordinates
    Point m_rubberBandCurrent;   // screen coordinates
    float m_rubberBandScrollOffsetY = 0.0f;

    // Middle-click browser-style autoscroll
    bool m_middleScrollActive = false;
    Point m_middleOrigin{};
    Point m_middleLastMouse{};

    // Auto-scroll state
    float m_autoScrollLastMouseX = 0.0f;
    float m_autoScrollLastMouseY = 0.0f;
    int m_autoScrollDirY = 1;
    std::chrono::steady_clock::time_point m_lastAutoScrollTime{};

    // Scroll state
    float m_scrollY = 0.0f;
    float m_targetScrollY = 0.0f;
    float m_scrollX = 0.0f;
    float m_maxScrollY = 0.0f;
    float m_maxScrollX = 0.0f;
    AnimatedScalar m_scrollYAnim{ 0.0f };
    bool m_isDraggingScrollbar = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;
    ScrollbarAutoHide m_scrollbarAutoHide;

    float m_headerHeight = 32.0f;
    float m_rowHeight = 28.0f;
    bool m_showGridLines = true;

    RenderLayer m_rowsLayer;
    bool m_rowsLayerCachesFull = false;
    static constexpr float kMaxFullContentCacheHeight = 4096.0f;
    static constexpr float kMaxFullContentCacheWidth = 4096.0f;

    Event<ListView*, int> m_onSelectionChangedEvent;
    Event<ListView*, int> m_onRowDoubleClickedEvent;
    Event<ListView*, int, bool> m_onColumnSortEvent;

    // Selection reveal ripple (Button-style expand-to-cover).
    int m_selectRippleRow = -1;
    float m_selectRippleLocalX = 0.0f;
    float m_selectRippleLocalY = 0.0f;
    float m_selectRippleRadius = 0.0f;
    bool m_selectRippleActive = false;
    bool m_selectRippleCovered = false;
};

} // namespace CUI
