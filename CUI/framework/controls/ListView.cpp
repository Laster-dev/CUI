#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ListView.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace CUI {

ListView::ListView() {
    SetProperty("background", Value(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f, 1.0f))); // VS Code Code Editor Dark #1E1E1E
    SetProperty("headerBackground", Value(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    SetProperty("gridLineBrush", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("color", Value(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("selectedBackground", Value(D2D1::ColorF(0x04 / 255.0f, 0x39 / 255.0f, 0x61 / 255.0f, 1.0f))); // VS Code Blue Selection
    SetProperty("hoverBackground", Value(D2D1::ColorF(0x2A / 255.0f, 0x2D / 255.0f, 0x2E / 255.0f, 1.0f)));
    SetProperty("fontSize", Value(12.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("width", Value(480.0f));
    SetProperty("height", Value(320.0f));
}

HCURSOR ListView::GetCursor() const {
    if (!IsEnabled()) return nullptr;
    if (m_hoveredColumnSplitter != -1 || m_isResizingColumn) {
        return LoadCursor(nullptr, IDC_SIZEWE);
    }
    return LoadCursor(nullptr, IDC_ARROW);
}

void ListView::AddColumn(const std::string& header, float width) {
    ListViewColumn col;
    col.header = header;
    col.width = std::max(col.minWidth, width);
    m_columns.push_back(col);
}

void ListView::ClearColumns() {
    m_columns.clear();
}

void ListView::AddRow(const std::vector<std::string>& rowData) {
    m_rows.push_back(rowData);
}

void ListView::SetRows(const std::vector<std::vector<std::string>>& rowsData) {
    m_rows = rowsData;
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_scrollY = 0.0f;
}

void ListView::ClearRows() {
    m_rows.clear();
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_scrollY = 0.0f;
}

void ListView::SelectAll() {
    m_selectedIndices.clear();
    for (size_t i = 0; i < m_rows.size(); ++i) {
        m_selectedIndices.insert(static_cast<int>(i));
    }
    m_onSelectionChangedEvent.Invoke(this, -1);
}

void ListView::ClearSelection() {
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_onSelectionChangedEvent.Invoke(this, -1);
}

void ListView::SetRowSelected(int rowIndex, bool selected) {
    if (rowIndex >= 0 && rowIndex < static_cast<int>(m_rows.size())) {
        if (selected) {
            m_selectedIndices.insert(rowIndex);
        } else {
            m_selectedIndices.erase(rowIndex);
        }
        m_onSelectionChangedEvent.Invoke(this, rowIndex);
    }
}

bool ListView::IsRowSelected(int rowIndex) const {
    return m_selectedIndices.find(rowIndex) != m_selectedIndices.end();
}

float ListView::GetTotalColumnsWidth() const {
    float totalW = 0.0f;
    for (const auto& col : m_columns) {
        totalW += col.width;
    }
    return totalW;
}

Size ListView::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(480.0f);
    float expH = GetProperty("height").AsFloat(320.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ListView::ClampScroll() {
    float totalContentH = m_rowHeight * m_rows.size();
    float viewH = m_bounds.height - m_headerHeight - 4.0f;
    m_maxScrollY = std::max(0.0f, totalContentH - viewH);
    m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScrollY);

    float totalColsW = GetTotalColumnsWidth();
    float viewW = m_bounds.width - 4.0f;
    m_maxScrollX = std::max(0.0f, totalColsW - viewW);
    m_scrollX = std::clamp(m_scrollX, 0.0f, m_maxScrollX);
}

int ListView::GetRowIndexFromY(float y) const {
    float contentY = y - m_bounds.y - m_headerHeight + m_scrollY;
    if (contentY < 0.0f) return -1;
    int idx = static_cast<int>(contentY / m_rowHeight);
    if (idx >= 0 && idx < static_cast<int>(m_rows.size())) {
        return idx;
    }
    return -1;
}

void ListView::UpdateRubberBandSelection() {
    if (!m_isRubberBandSelecting) return;

    // Calculate rubber-band selection rectangle in container space
    float minX = std::min(m_rubberBandStart.x, m_rubberBandCurrent.x);
    float maxX = std::max(m_rubberBandStart.x, m_rubberBandCurrent.x);
    float minY = std::min(m_rubberBandStart.y, m_rubberBandCurrent.y);
    float maxY = std::max(m_rubberBandStart.y, m_rubberBandCurrent.y);

    // Determine row index range covered by rubber-band Y coordinates
    int startRow = GetRowIndexFromY(minY);
    int endRow = GetRowIndexFromY(maxY);

    if (startRow == -1 && minY < m_bounds.y + m_headerHeight) startRow = 0;
    if (endRow == -1 && maxY > m_bounds.y + m_headerHeight) endRow = static_cast<int>(m_rows.size()) - 1;

    m_selectedIndices.clear();
    if (startRow != -1 && endRow != -1) {
        int r1 = std::min(startRow, endRow);
        int r2 = std::max(startRow, endRow);
        for (int r = r1; r <= r2; ++r) {
            if (r >= 0 && r < static_cast<int>(m_rows.size())) {
                m_selectedIndices.insert(r);
            }
        }
    }
    m_onSelectionChangedEvent.Invoke(this, -1);
}

void ListView::OnRender(GraphicsContext& ctx) {
    ClampScroll();

    float radius = GetProperty("cornerRadius").AsFloat(0.0f);
    D2D1_COLOR_F bg = GetProperty("background").AsColor(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f, 1.0f));
    D2D1_COLOR_F headerBg = GetProperty("headerBackground").AsColor(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f));
    D2D1_COLOR_F borderClr = GetProperty("borderBrush").AsColor(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f));
    D2D1_COLOR_F gridLineClr = GetProperty("gridLineBrush").AsColor(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f));
    D2D1_COLOR_F textClr = GetProperty("color").AsColor(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f));
    D2D1_COLOR_F selectedBg = GetProperty("selectedBackground").AsColor(D2D1::ColorF(0x04 / 255.0f, 0x39 / 255.0f, 0x61 / 255.0f, 1.0f));
    D2D1_COLOR_F hoverBg = GetProperty("hoverBackground").AsColor(D2D1::ColorF(0x2A / 255.0f, 0x2D / 255.0f, 0x2E / 255.0f, 1.0f));
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontH = GetProperty("fontSize").AsFloat(12.0f);

    // Draw container outer box
    ctx.FillRect(m_bounds, bg);
    ctx.DrawRect(m_bounds, borderClr, 1.0f);

    // 1. Render Multi-Column Header Bar
    Rect headerRect(m_bounds.x, m_bounds.y, m_bounds.width, m_headerHeight);
    ctx.FillRect(headerRect, headerBg);
    ctx.DrawLine(Point(m_bounds.x, m_bounds.y + m_headerHeight), Point(m_bounds.x + m_bounds.width, m_bounds.y + m_headerHeight), borderClr, 1.0f);

    float currColX = m_bounds.x - m_scrollX;
    ctx.PushClip(Rect(m_bounds.x + 1, m_bounds.y + 1, m_bounds.width - 2, m_headerHeight - 1));

    for (size_t colIdx = 0; colIdx < m_columns.size(); ++colIdx) {
        const auto& col = m_columns[colIdx];
        Rect colHeaderRect(currColX, m_bounds.y, col.width, m_headerHeight);

        // Header column text
        Rect colTextRect(colHeaderRect.x + 8.0f, colHeaderRect.y, colHeaderRect.width - 16.0f, colHeaderRect.height);
        ctx.DrawText(col.header, colTextRect, D2D1::ColorF(0xE0 / 255.0f, 0xE0 / 255.0f, 0xE0 / 255.0f), font, fontH, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Header column splitter line
        ctx.DrawLine(Point(currColX + col.width, m_bounds.y + 4.0f), Point(currColX + col.width, m_bounds.y + m_headerHeight - 4.0f), borderClr, 1.0f);

        currColX += col.width;
    }
    ctx.PopClip();

    // 2. High-Performance Virtualized Rows Area
    Rect contentArea(m_bounds.x + 1.0f, m_bounds.y + m_headerHeight + 1.0f, m_bounds.width - 2.0f, m_bounds.height - m_headerHeight - 2.0f);
    ctx.PushClip(contentArea);

    int startRow = std::max(0, static_cast<int>(m_scrollY / m_rowHeight));
    int endRow = std::min(static_cast<int>(m_rows.size()) - 1, static_cast<int>((m_scrollY + contentArea.height) / m_rowHeight));

    float totalColsW = GetTotalColumnsWidth();

    for (int r = startRow; r <= endRow; ++r) {
        float rowY = m_bounds.y + m_headerHeight + r * m_rowHeight - m_scrollY;
        Rect rowRect(m_bounds.x, rowY, std::max(m_bounds.width, totalColsW), m_rowHeight);

        bool isSelected = IsRowSelected(r);
        bool isHovered = (r == m_hoveredRowIndex);

        // Row background state
        if (isSelected) {
            ctx.FillRect(rowRect, selectedBg);
        } else if (isHovered && IsEnabled()) {
            ctx.FillRect(rowRect, hoverBg);
        }

        // Draw Row Grid Line
        ctx.DrawLine(Point(m_bounds.x, rowY + m_rowHeight), Point(m_bounds.x + m_bounds.width, rowY + m_rowHeight), gridLineClr, 1.0f);

        // Draw Cells Data
        float cellX = m_bounds.x - m_scrollX;
        const auto& rowData = m_rows[r];

        for (size_t c = 0; c < m_columns.size(); ++c) {
            float colW = m_columns[c].width;
            if (c < rowData.size()) {
                Rect cellRect(cellX + 8.0f, rowY, colW - 16.0f, m_rowHeight);
                D2D1_COLOR_F cellClr = isSelected ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : textClr;
                ctx.DrawText(rowData[c], cellRect, cellClr, font, fontH, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
            // Cell Vertical Grid Line
            ctx.DrawLine(Point(cellX + colW, rowY), Point(cellX + colW, rowY + m_rowHeight), gridLineClr, 1.0f);
            cellX += colW;
        }
    }

    // 3. Draw Rubber-Band Selection Box (拉框选择虚线框)
    if (m_isRubberBandSelecting) {
        float rx = std::min(m_rubberBandStart.x, m_rubberBandCurrent.x);
        float ry = std::min(m_rubberBandStart.y, m_rubberBandCurrent.y);
        float rw = std::abs(m_rubberBandCurrent.x - m_rubberBandStart.x);
        float rh = std::abs(m_rubberBandCurrent.y - m_rubberBandStart.y);

        Rect rubberRect(rx, ry, rw, rh);
        ctx.FillRect(rubberRect, D2D1::ColorF(0.0f, 0.48f, 0.80f, 0.25f)); // Translucent Blue
        ctx.DrawRect(rubberRect, D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f), 1.0f);
    }

    // 4. Draw Vertical & Horizontal ScrollBars
    if (m_maxScrollY > 0.0f) {
        float trackX = m_bounds.x + m_bounds.width - 8.0f;
        float trackY = m_bounds.y + m_headerHeight + 2.0f;
        float trackH = m_bounds.height - m_headerHeight - 4.0f;

        float contentH = m_rowHeight * m_rows.size();
        float thumbH = std::max(20.0f, trackH * (trackH / contentH));
        float thumbY = trackY + (m_scrollY / m_maxScrollY) * (trackH - thumbH);

        Rect thumbRect(trackX, thumbY, 6.0f, thumbH);
        ctx.FillRoundedRect(thumbRect, 3.0f, D2D1::ColorF(0x66 / 255.0f, 0x66 / 255.0f, 0x66 / 255.0f, 0.6f));
    }

    ctx.PopClip();
}

void ListView::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    // 1. Check Column Resizing Splitters in Header Bar
    if (pt.y >= m_bounds.y && pt.y <= m_bounds.y + m_headerHeight) {
        float currX = m_bounds.x - m_scrollX;
        for (size_t c = 0; c < m_columns.size(); ++c) {
            float splitterX = currX + m_columns[c].width;
            if (std::abs(pt.x - splitterX) <= 5.0f && m_columns[c].isResizable) {
                m_isResizingColumn = true;
                m_resizingColumnIndex = static_cast<int>(c);
                m_dragStartX = pt.x;
                m_initialColumnWidth = m_columns[c].width;
                return;
            }
            currX += m_columns[c].width;
        }
        return;
    }

    // 2. Row Click & Selection Management
    int clickedRow = GetRowIndexFromY(pt.y);
    if (clickedRow >= 0) {
        bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (m_selectionMode == ListViewSelectionMode::Single || (!ctrlDown && !shiftDown)) {
            m_selectedIndices.clear();
            m_selectedIndices.insert(clickedRow);
            m_anchorIndex = clickedRow;
        } else if (ctrlDown) {
            if (IsRowSelected(clickedRow)) {
                m_selectedIndices.erase(clickedRow);
            } else {
                m_selectedIndices.insert(clickedRow);
            }
            m_anchorIndex = clickedRow;
        } else if (shiftDown && m_anchorIndex != -1) {
            m_selectedIndices.clear();
            int r1 = std::min(m_anchorIndex, clickedRow);
            int r2 = std::max(m_anchorIndex, clickedRow);
            for (int r = r1; r <= r2; ++r) {
                m_selectedIndices.insert(r);
            }
        }
        m_onSelectionChangedEvent.Invoke(this, clickedRow);
    } else {
        // Start Rubber-Band Drag Selection in empty area
        m_isRubberBandSelecting = true;
        m_rubberBandStart = pt;
        m_rubberBandCurrent = pt;
    }
}

void ListView::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);

    // 1. Column Resizing Drag
    if (m_isResizingColumn && m_resizingColumnIndex >= 0 && m_resizingColumnIndex < static_cast<int>(m_columns.size())) {
        float deltaX = pt.x - m_dragStartX;
        float newW = std::max(m_columns[m_resizingColumnIndex].minWidth, m_initialColumnWidth + deltaX);
        m_columns[m_resizingColumnIndex].width = newW;
        return;
    }

    // 2. Rubber-Band Drag Selection
    if (m_isRubberBandSelecting && m_isPressed) {
        m_rubberBandCurrent = pt;
        UpdateRubberBandSelection();
        return;
    }

    // 3. Hover state on Column Splitter lines
    m_hoveredColumnSplitter = -1;
    if (pt.y >= m_bounds.y && pt.y <= m_bounds.y + m_headerHeight) {
        float currX = m_bounds.x - m_scrollX;
        for (size_t c = 0; c < m_columns.size(); ++c) {
            float splitterX = currX + m_columns[c].width;
            if (std::abs(pt.x - splitterX) <= 5.0f && m_columns[c].isResizable) {
                m_hoveredColumnSplitter = static_cast<int>(c);
                break;
            }
            currX += m_columns[c].width;
        }
    }

    m_hoveredRowIndex = GetRowIndexFromY(pt.y);
}

void ListView::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    m_isResizingColumn = false;
    m_isRubberBandSelecting = false;
}

void ListView::OnMouseWheel(float delta) {
    bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    if (shiftDown) {
        m_scrollX -= delta * 40.0f;
    } else {
        m_scrollY -= delta * m_rowHeight * 3.0f;
    }
    ClampScroll();
}

void ListView::OnKeyDown(int vkCode) {
    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    if (ctrlDown && vkCode == 'A') {
        SelectAll();
        return;
    }

    if (m_rows.empty()) return;

    int activeRow = (m_anchorIndex != -1) ? m_anchorIndex : 0;
    int newRow = activeRow;

    switch (vkCode) {
    case VK_UP:
        newRow = std::max(0, activeRow - 1);
        break;
    case VK_DOWN:
        newRow = std::min(static_cast<int>(m_rows.size()) - 1, activeRow + 1);
        break;
    }

    if (newRow != activeRow) {
        m_anchorIndex = newRow;
        m_selectedIndices.clear();
        m_selectedIndices.insert(newRow);
        m_onSelectionChangedEvent.Invoke(this, newRow);
    }
}

} // namespace CUI
