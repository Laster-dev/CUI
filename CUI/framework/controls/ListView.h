#pragma once
#include "Control.h"
#include <vector>
#include <string>
#include <unordered_set>

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

class ListView : public Control {
public:
    ListView();
    virtual ~ListView() = default;

    virtual const char* GetClassName() const override { return "ListView"; }
    virtual HCURSOR GetCursor() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnMouseWheel(float delta) override;

    // Columns Management
    void AddColumn(const std::string& header, float width = 120.0f);
    void ClearColumns();
    const std::vector<ListViewColumn>& GetColumns() const { return m_columns; }

    // Data Rows Management
    void AddRow(const std::vector<std::string>& rowData);
    void SetRows(const std::vector<std::vector<std::string>>& rowsData);
    void ClearRows();
    size_t GetRowCount() const { return m_rows.size(); }

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

    std::vector<ListViewColumn> m_columns;
    std::vector<std::vector<std::string>> m_rows;

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
    Point m_rubberBandStart;
    Point m_rubberBandCurrent;

    // Scroll state
    float m_scrollY = 0.0f;
    float m_scrollX = 0.0f;
    float m_maxScrollY = 0.0f;
    float m_maxScrollX = 0.0f;

    float m_headerHeight = 32.0f;
    float m_rowHeight = 28.0f;

    Event<ListView*, int> m_onSelectionChangedEvent;
    Event<ListView*, int> m_onRowDoubleClickedEvent;
};

} // namespace CUI
