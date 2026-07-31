#pragma once
#include "Control.h"
#include "ChromiumScrollAnimator.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <functional>
#include <cstdint>

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
// Usage: SetVirtualMode(100000, callback) - callback returns cell text per (row, col)
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
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnMouseWheel(float delta) override;
    virtual void OnAutoScrollTick() override;
    virtual bool NeedsAutoScrollTick() const override { return m_isRubberBandSelecting; }
    virtual bool OnAnimationTick() override;

    // Columns Management
    void AddColumn(const std::string& header, float width = 120.0f);
    void ClearColumns();
    const std::vector<ListViewColumn>& GetColumns() const { return m_columns; }

    // In-Memory Data Rows Management (for small-medium datasets)
    void AddRow(const std::vector<std::string>& rowData);
    void AddRow(const std::vector<ListViewCellData>& rowData);
    void SetRows(const std::vector<std::vector<std::string>>& rowsData);
    void SetRows(const std::vector<std::vector<ListViewCellData>>& rowsData);
    void ClearRows();
    size_t GetRowCount() const;

    // Virtual Mode for 100k+ rows (high performance)
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

    // Events
    Event<ListView*, int>& OnSelectionChanged() { return m_onSelectionChangedEvent; }
    Event<ListView*, int>& OnRowDoubleClicked() { return m_onRowDoubleClickedEvent; }

private:
    void ClampScroll();
    int GetRowIndexFromY(float y) const;
    int GetColumnIndexFromX(float x) const;
    void UpdateRubberBandSelection();
    float GetTotalColumnsWidth() const;
    // Applies auto-scroll based on last mouse position.
    // Does not call ClampScroll() / UpdateRubberBandSelection().
    bool ApplyAutoScroll();
    void StopSmoothScroll();
    bool AdvanceSmoothScroll();
    double SecondsSinceLastTick();

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
    int m_hoveredRowIndex = -1;
    int m_hoveredColumnSplitter = -1;

    // Column resizing state
    bool m_isResizingColumn = false;
    int m_resizingColumnIndex = -1;
    float m_dragStartX = 0.0f;
    float m_initialColumnWidth = 0.0f;

    // Rubber-band selection state (拉框选择)
    bool m_isMouseDown = false;
    Point m_mouseDownPoint;
    int m_pendingRowClick = -1;
    std::unordered_set<int> m_initialSelectedBeforeDrag;

    bool m_isRubberBandSelecting = false;
    Point m_rubberBandStart;     // content coordinates (scroll-adjusted)
    Point m_rubberBandCurrent;   // screen coordinates
    float m_rubberBandScrollOffsetY = 0.0f; // snap scrollY at drag start

    // Auto-scroll state
    float m_autoScrollLastMouseX = 0.0f;
    float m_autoScrollLastMouseY = 0.0f;
    // Used to avoid rapid direction flip (jitter) around the midline.
    // +1: auto-scroll down, -1: auto-scroll up.
    int m_autoScrollDirY = 1;
    // Time normalization for ApplyAutoScroll() (timer tick vs mouse-move calls).
    std::uint64_t m_lastAutoScrollMs = 0;

    // Scroll state
    float m_scrollY = 0.0f;
    float m_scrollX = 0.0f;
    float m_maxScrollY = 0.0f;
    float m_maxScrollX = 0.0f;
    ChromiumScrollAnimator m_scrollAnimator;
    bool m_isDraggingScrollbar = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;
    LARGE_INTEGER m_qpcFreq = {};
    LONGLONG m_lastAnimQpc = 0;

    float m_headerHeight = 32.0f;
    float m_rowHeight = 28.0f;

    Event<ListView*, int> m_onSelectionChangedEvent;
    Event<ListView*, int> m_onRowDoubleClickedEvent;
};

} // namespace CUI
