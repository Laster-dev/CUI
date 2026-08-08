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
};

class ListView : public Control {
public:
    ListView();
    virtual ~ListView() = default;

    virtual const char* GetClassName() const override { return "ListView"; }
    virtual HCURSOR GetCursor() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual UIElement* HitTest(float x, float y) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnMouseWheel(float delta) override;
    virtual void OnAutoScrollTick() override;
    virtual bool NeedsAutoScrollTick() const override { return m_isRubberBandSelecting; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnThemeChanged() override;

    // Columns Management
    void AddColumn(const std::string& header, float width = 120.0f);
    void ClearColumns();
    const std::vector<ListViewColumn>& GetColumns() const { return m_columns; }

    // In-Memory Data Rows Management
    void AddRow(const std::vector<std::string>& rowData);
    void AddRow(const std::vector<ListViewCellData>& rowData);
    void SetRows(const std::vector<std::vector<std::string>>& rowsData);
    void SetRows(const std::vector<std::vector<ListViewCellData>>& rowsData);
    void ClearRows();
    size_t GetRowCount() const;

    // Virtual Mode
    void SetVirtualMode(int rowCount, ListViewDataSource* dataSource);
    // Row Height
    float GetRowHeight() const { return m_rowHeight; }
    void SetRowHeight(float h) { m_rowHeight = h; }

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

    // Events
    Event<ListView*, int>& OnSelectionChanged() { return m_onSelectionChangedEvent; }
    Event<ListView*, int>& OnRowDoubleClicked() { return m_onRowDoubleClickedEvent; }

private:
    void ClampScroll();
    int GetRowIndexFromY(float y) const;
    int GetColumnIndexFromX(float x) const;
    void UpdateRubberBandSelection();
    float GetTotalColumnsWidth() const;
    bool ApplyAutoScroll();
    void SelectRange(int fromIdx, int toIdx, bool keepExisting = false);
    void InvalidateRowsLayer();
    bool CanCacheFullRows() const;
    float GetRowsContentHeight() const;
    Rect GetRowsViewportRect() const;
    void PaintRowsRange(GraphicsContext& ctx, int startRow, int endRow, float scrollX, float scrollY);
    void RenderRowsLayer(GraphicsContext& ctx);

    std::string GetCellText(int row, int col) const;
    std::shared_ptr<UIElement> GetCellElement(int row, int col) const;

    std::vector<ListViewColumn> m_columns;
    std::vector<std::vector<ListViewCellData>> m_rows;

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
    Point m_mouseDownPoint;
    int m_pendingRowClick = -1;
    std::unordered_set<int> m_initialSelectedBeforeDrag;

    bool m_isRubberBandSelecting = false;
    Point m_rubberBandStart;     // content coordinates
    Point m_rubberBandCurrent;   // screen coordinates
    float m_rubberBandScrollOffsetY = 0.0f;

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

    RenderLayer m_rowsLayer;
    bool m_rowsLayerCachesFull = false;
    static constexpr float kMaxFullContentCacheHeight = 4096.0f;
    static constexpr float kMaxFullContentCacheWidth = 4096.0f;

    Event<ListView*, int> m_onSelectionChangedEvent;
    Event<ListView*, int> m_onRowDoubleClickedEvent;
};

} // namespace CUI
