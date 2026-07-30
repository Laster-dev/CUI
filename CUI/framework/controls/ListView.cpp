#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ListView.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace CUI {

namespace {
float GetChromiumWheelStep(float viewportHeight) {
    UINT lines = 3;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    if (lines == WHEEL_PAGESCROLL) {
        return (std::max)(40.0f, viewportHeight);
    }
    return (std::max)(1u, lines) * 40.0f;
}
}

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
    QueryPerformanceFrequency(&m_qpcFreq);
    m_scrollAnimator.Reset(0.0f);
}

void ListView::StopSmoothScroll() {
    m_scrollAnimator.JumpTo(m_scrollY);
    m_lastAnimQpc = 0;
}

double ListView::SecondsSinceLastTick() {
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);

    if (m_lastAnimQpc == 0 || m_qpcFreq.QuadPart <= 0) {
        m_lastAnimQpc = now.QuadPart;
        return 1.0 / 120.0;
    }

    double dt = static_cast<double>(now.QuadPart - m_lastAnimQpc) / static_cast<double>(m_qpcFreq.QuadPart);
    m_lastAnimQpc = now.QuadPart;
    if (dt < 0.0005) dt = 0.0005;
    if (dt > 0.050) dt = 0.050;
    return dt;
}

bool ListView::AdvanceSmoothScroll() {
    if (m_isDraggingScrollbar || m_isRubberBandSelecting) {
        StopSmoothScroll();
        return false;
    }

    double dt = SecondsSinceLastTick();
    bool advanced = m_scrollAnimator.Tick(dt, 0.0f, m_maxScrollY);
    if (!advanced) {
        m_lastAnimQpc = 0;
        return false;
    }

    m_scrollY = m_scrollAnimator.Current();
    ClampScroll();
    return true;
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
    std::vector<ListViewCellData> cellRow;
    cellRow.reserve(rowData.size());
    for (const auto& s : rowData) {
        cellRow.push_back({ s, nullptr });
    }
    m_rows.push_back(cellRow);
}

void ListView::AddRow(const std::vector<ListViewCellData>& rowData) {
    m_rows.push_back(rowData);
    for (const auto& cell : rowData) {
        if (cell.customElement) AddChild(cell.customElement);
    }
}

void ListView::SetRows(const std::vector<std::vector<std::string>>& rowsData) {
    ClearChildren();
    m_rows.clear();
    for (const auto& row : rowsData) {
        AddRow(row);
    }
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_scrollY = 0.0f;
    m_scrollAnimator.JumpTo(0.0f);
}

void ListView::SetRows(const std::vector<std::vector<ListViewCellData>>& rowsData) {
    ClearChildren();
    m_rows.clear();
    for (const auto& row : rowsData) {
        AddRow(row);
    }
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_scrollY = 0.0f;
    m_scrollAnimator.JumpTo(0.0f);
}

void ListView::ClearRows() {
    ClearChildren();
    m_rows.clear();
    m_virtualMode = false;
    m_virtualRowCount = 0;
    m_dataSource = nullptr;
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_scrollY = 0.0f;
    m_scrollAnimator.JumpTo(0.0f);
}

size_t ListView::GetRowCount() const {
    if (m_virtualMode) return static_cast<size_t>(m_virtualRowCount);
    return m_rows.size();
}

void ListView::SetVirtualMode(int rowCount, ListViewDataSource* dataSource) {
    ClearChildren();
    m_rows.clear();
    m_virtualMode = true;
    m_virtualRowCount = rowCount;
    m_dataSource = dataSource;
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_scrollY = 0.0f;
    m_scrollAnimator.JumpTo(0.0f);
}

std::string ListView::GetCellText(int row, int col) const {
    if (m_virtualMode) {
        if (m_dataSource) return m_dataSource->GetCellText(row, col);
        return "";
    }
    if (row >= 0 && row < static_cast<int>(m_rows.size())) {
        if (col >= 0 && col < static_cast<int>(m_rows[row].size())) {
            return m_rows[row][col].text;
        }
    }
    return "";
}

std::shared_ptr<UIElement> ListView::GetCellElement(int row, int col) const {
    if (m_virtualMode) {
        if (m_dataSource) return m_dataSource->GetCellElement(row, col);
        return nullptr;
    }
    if (row >= 0 && row < static_cast<int>(m_rows.size())) {
        if (col >= 0 && col < static_cast<int>(m_rows[row].size())) {
            return m_rows[row][col].customElement;
        }
    }
    return nullptr;
}

void ListView::SelectAll() {
    m_selectedIndices.clear();
    int rowCount = static_cast<int>(GetRowCount());
    for (int i = 0; i < rowCount; ++i) {
        m_selectedIndices.insert(i);
    }
    m_onSelectionChangedEvent.Invoke(this, -1);
}

void ListView::ClearSelection() {
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_onSelectionChangedEvent.Invoke(this, -1);
}

void ListView::SetRowSelected(int rowIndex, bool selected) {
    if (rowIndex >= 0 && rowIndex < static_cast<int>(GetRowCount())) {
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
    float totalContentH = m_rowHeight * static_cast<float>(GetRowCount());
    float viewH = m_bounds.height - m_headerHeight - 4.0f;
    m_maxScrollY = std::max(0.0f, totalContentH - viewH);
    m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScrollY);
    m_scrollAnimator.ClampTo(0.0f, m_maxScrollY);

    float totalColsW = GetTotalColumnsWidth();
    float viewW = m_bounds.width - 4.0f;
    m_maxScrollX = std::max(0.0f, totalColsW - viewW);
    m_scrollX = std::clamp(m_scrollX, 0.0f, m_maxScrollX);
}

int ListView::GetRowIndexFromY(float y) const {
    float contentY = y - m_bounds.y - m_headerHeight + m_scrollY;
    if (contentY < 0.0f) return -1;
    int idx = static_cast<int>(contentY / m_rowHeight);
    int rowCount = static_cast<int>(GetRowCount());
    if (idx >= 0 && idx < rowCount) {
        return idx;
    }
    return -1;
}

void ListView::UpdateRubberBandSelection() {
    if (!m_isRubberBandSelecting) return;

    // m_rubberBandStart is in content coords; m_rubberBandCurrent is screen coords
    // Convert current mouse to content coordinates
    float currentContentX = m_rubberBandCurrent.x - m_bounds.x + m_scrollX;
    float currentContentY = m_rubberBandCurrent.y - m_bounds.y - m_headerHeight + m_scrollY;

    // Selection rectangle in content coordinates
    float minContentX = std::min(m_rubberBandStart.x, currentContentX);
    float maxContentX = std::max(m_rubberBandStart.x, currentContentX);
    float minContentY = std::min(m_rubberBandStart.y, currentContentY);
    float maxContentY = std::max(m_rubberBandStart.y, currentContentY);

    // Determine row index range from content Y coords
    int startRow = (minContentY >= 0.0f) ? static_cast<int>(minContentY / m_rowHeight) : 0;
    int endRow = (maxContentY >= 0.0f) ? static_cast<int>(maxContentY / m_rowHeight) : 0;

    int rowCount = static_cast<int>(GetRowCount());
    startRow = std::clamp(startRow, 0, rowCount - 1);
    endRow = std::clamp(endRow, 0, rowCount - 1);

    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (ctrlDown) {
        m_selectedIndices = m_initialSelectedBeforeDrag;
    } else {
        m_selectedIndices.clear();
    }

    int r1 = std::min(startRow, endRow);
    int r2 = std::max(startRow, endRow);
    for (int r = r1; r <= r2; ++r) {
        if (r >= 0 && r < rowCount) {
            m_selectedIndices.insert(r);
        }
    }
    m_onSelectionChangedEvent.Invoke(this, -1);
}

void ListView::Render(GraphicsContext& ctx) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return;

    bool clip = ShouldClipToBounds();
    if (clip) {
        ctx.PushClip(m_bounds);
    }

    OnRender(ctx);

    if (clip) {
        ctx.PopClip();
    }
}

UIElement* ListView::HitTest(float x, float y) {
    std::string visStr = GetProperty("visibility").AsString("Visible");
    if (visStr != "Visible") return nullptr;

    if (m_bounds.Contains(x, y)) {
        int r = GetRowIndexFromY(y);
        int rowCount = static_cast<int>(GetRowCount());
        if (r >= 0 && r < rowCount) {
            float rowY = m_bounds.y + m_headerHeight + r * m_rowHeight - m_scrollY;
            float cellX = m_bounds.x - m_scrollX;

            for (size_t c = 0; c < m_columns.size(); ++c) {
                float colW = m_columns[c].width;
                auto cellElem = GetCellElement(r, static_cast<int>(c));
                if (cellElem) {
                    Rect cellRect(cellX + 2.0f, rowY + 2.0f, colW - 4.0f, m_rowHeight - 4.0f);
                    cellElem->Measure(Size(cellRect.width, cellRect.height));
                    cellElem->Arrange(cellRect);

                    UIElement* childHit = cellElem->HitTest(x, y);
                    if (childHit && childHit != cellElem.get()) return childHit;
                }
                cellX += colW;
            }
        }
        return this;
    }
    return nullptr;
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

    int rowCount = static_cast<int>(GetRowCount());
    int startRow = std::max(0, static_cast<int>(m_scrollY / m_rowHeight));
    int endRow = std::min(rowCount - 1, static_cast<int>((m_scrollY + contentArea.height) / m_rowHeight));

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

        for (size_t c = 0; c < m_columns.size(); ++c) {
            float colW = m_columns[c].width;
            auto cellElem = GetCellElement(r, static_cast<int>(c));
            if (cellElem) {
                Rect cellRect(cellX + 2.0f, rowY + 2.0f, colW - 4.0f, m_rowHeight - 4.0f);
                cellElem->Measure(Size(cellRect.width, cellRect.height));
                cellElem->Arrange(cellRect);
                cellElem->Render(ctx);
            } else {
                std::string cellText = GetCellText(r, static_cast<int>(c));
                Rect cellRect(cellX + 8.0f, rowY, colW - 16.0f, m_rowHeight);
                D2D1_COLOR_F cellClr = isSelected ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : textClr;
                ctx.DrawText(cellText, cellRect, cellClr, font, fontH, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
            // Cell Vertical Grid Line
            ctx.DrawLine(Point(cellX + colW, rowY), Point(cellX + colW, rowY + m_rowHeight), gridLineClr, 1.0f);
            cellX += colW;
        }
    }

    // 3. Draw Rubber-Band Selection Box (拉框选择虚线框)
    if (m_isRubberBandSelecting) {
        // Current mouse in content coords
        float curContentX = m_rubberBandCurrent.x - m_bounds.x + m_scrollX;
        float curContentY = m_rubberBandCurrent.y - m_bounds.y - m_headerHeight + m_scrollY;

        // Visible content Y range
        float visTop = m_scrollY;
        float visBottom = m_scrollY + m_bounds.height - m_headerHeight;

        // Clamp both start & current to visible content range
        float drawTop = std::clamp(std::min(m_rubberBandStart.y, curContentY), visTop, visBottom);
        float drawBottom = std::clamp(std::max(m_rubberBandStart.y, curContentY), visTop, visBottom);

        // Convert to screen Y
        float screenTop = m_bounds.y + m_headerHeight + drawTop - m_scrollY;
        float screenBottom = m_bounds.y + m_headerHeight + drawBottom - m_scrollY;

        // X: clamp to content width
        float drawLeft = std::clamp(std::min(m_rubberBandStart.x, curContentX), 0.0f, GetTotalColumnsWidth());
        float drawRight = std::clamp(std::max(m_rubberBandStart.x, curContentX), 0.0f, GetTotalColumnsWidth());
        float screenLeft = m_bounds.x + drawLeft - m_scrollX;
        float screenRight = m_bounds.x + drawRight - m_scrollX;

        if (screenBottom > screenTop && screenRight > screenLeft) {
            Rect rubberRect(screenLeft, screenTop, screenRight - screenLeft, screenBottom - screenTop);
            ctx.FillRect(rubberRect, D2D1::ColorF(0.0f, 0.48f, 0.80f, 0.25f));
            ctx.DrawRect(rubberRect, D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f), 1.0f);
        }
    }

    // 4. Draw Vertical & Horizontal ScrollBars
    if (m_maxScrollY > 0.0f) {
        float trackX = m_bounds.x + m_bounds.width - 8.0f;
        float trackY = m_bounds.y + m_headerHeight + 2.0f;
        float trackH = m_bounds.height - m_headerHeight - 4.0f;

        float contentH = m_rowHeight * static_cast<float>(GetRowCount());
        float thumbH = std::max(20.0f, trackH * (trackH / contentH));
        float thumbY = trackY + (m_scrollY / m_maxScrollY) * (trackH - thumbH);

        Rect thumbRect(trackX, thumbY, 6.0f, thumbH);
        ctx.FillRoundedRect(thumbRect, 3.0f, D2D1::ColorF(0x66 / 255.0f, 0x66 / 255.0f, 0x66 / 255.0f, 0.6f));
    }

    ctx.PopClip();
}

void ListView::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    StopSmoothScroll();

    m_isMouseDown = true;
    m_mouseDownPoint = pt;
    m_initialSelectedBeforeDrag = m_selectedIndices;
    m_pendingRowClick = -1;

    // 1. Check Vertical ScrollBar Track / Thumb Click
    if (m_maxScrollY > 0.0f) {
        float trackX = m_bounds.x + m_bounds.width - 12.0f;
        if (pt.x >= trackX && pt.y >= m_bounds.y + m_headerHeight) {
            m_isDraggingScrollbar = true;
            m_dragStartY = pt.y;
            m_dragStartScrollY = m_scrollY;
            return;
        }
    }

    // 2. Check Column Resizing Splitters in Header Bar
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

    // Record pending row click
    int clickedRow = GetRowIndexFromY(pt.y);
    m_pendingRowClick = clickedRow;
}

void ListView::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);

    // 1. Check ScrollBar Dragging
    if (m_isDraggingScrollbar && m_isPressed) {
        float deltaY = pt.y - m_dragStartY;
        float trackH = m_bounds.height - m_headerHeight - 4.0f;
        float contentH = m_rowHeight * static_cast<float>(GetRowCount());
        float thumbH = std::max(20.0f, trackH * (trackH / contentH));
        float scrollableTrackH = trackH - thumbH;

        if (scrollableTrackH > 0.0f) {
            m_scrollY = m_dragStartScrollY + (deltaY / scrollableTrackH) * m_maxScrollY;
            ClampScroll();
            m_scrollAnimator.JumpTo(m_scrollY);
        }
        return;
    }

    // 2. Column Resizing Drag
    if (m_isResizingColumn && m_resizingColumnIndex >= 0 && m_resizingColumnIndex < static_cast<int>(m_columns.size())) {
        float deltaX = pt.x - m_dragStartX;
        float newW = std::max(m_columns[m_resizingColumnIndex].minWidth, m_initialColumnWidth + deltaX);
        m_columns[m_resizingColumnIndex].width = newW;
        return;
    }

    // 3. Check if mouse drag exceeds threshold to start Rubber-Band Marquee Selection
    if (m_isMouseDown && !m_isResizingColumn && !m_isDraggingScrollbar && !m_isRubberBandSelecting) {
        float dx = pt.x - m_mouseDownPoint.x;
        float dy = pt.y - m_mouseDownPoint.y;
        if (std::sqrt(dx * dx + dy * dy) > 4.0f) {
            m_isRubberBandSelecting = true;
            // Store start in content coordinates (scroll-adjusted)
            m_rubberBandStart.x = m_mouseDownPoint.x - m_bounds.x + m_scrollX;
            m_rubberBandStart.y = m_mouseDownPoint.y - m_bounds.y - m_headerHeight + m_scrollY;
            m_rubberBandScrollOffsetY = m_scrollY;

            // Initialize auto-scroll direction to reduce jitter around midline.
            float contentTop = m_bounds.y + m_headerHeight;
            float contentBottom = m_bounds.y + m_bounds.height;
            float contentH = contentBottom - contentTop;
            if (contentH > 0.0f) {
                float midY = contentTop + contentH * 0.5f;
                m_autoScrollDirY = (pt.y >= midY) ? 1 : -1; // +1 down, -1 up
            }
        }
    }

    // 4. Rubber-Band Drag Selection
    if (m_isRubberBandSelecting) {
        m_rubberBandCurrent = pt;
        m_autoScrollLastMouseX = pt.x;
        m_autoScrollLastMouseY = pt.y;
        bool scrolled = ApplyAutoScroll();
        if (scrolled) {
            ClampScroll();
        }
        UpdateRubberBandSelection();
        return;
    }

    // 5. Hover state on Column Splitter lines
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

    int rowCount = static_cast<int>(GetRowCount());

    // If mouse was clicked without dragging, perform normal row selection!
    if (!m_isRubberBandSelecting && !m_isResizingColumn && m_pendingRowClick != -1) {
        int clickedRow = m_pendingRowClick;
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
    } else if (!m_isRubberBandSelecting && !m_isResizingColumn && m_pendingRowClick == -1) {
        // Clicked empty area without dragging -> clear selection
        m_selectedIndices.clear();
        m_anchorIndex = -1;
        m_onSelectionChangedEvent.Invoke(this, -1);
    }

    m_isMouseDown = false;
    m_isResizingColumn = false;
    m_isRubberBandSelecting = false;
    m_isDraggingScrollbar = false;
    m_pendingRowClick = -1;
}

void ListView::OnMouseWheel(float delta) {
    bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    if (shiftDown) {
        m_scrollX -= delta * 40.0f;
        ClampScroll();
    } else {
        ClampScroll();
        m_scrollAnimator.ScrollBy(-delta * GetChromiumWheelStep(m_bounds.height - m_headerHeight), 0.0f, m_maxScrollY);
        if (m_lastAnimQpc == 0) {
            LARGE_INTEGER now = {};
            QueryPerformanceCounter(&now);
            m_lastAnimQpc = now.QuadPart;
        }
    }
}

bool ListView::ApplyAutoScroll() {
    float mouseX = m_autoScrollLastMouseX;
    float mouseY = m_autoScrollLastMouseY;
    bool scrolled = false;

    // Normalize speed by real time elapsed since last ApplyAutoScroll().
    // Otherwise, calling it from WM_MOUSEMOVE (more frequent) makes scrolling too fast.
    std::uint64_t nowMs = GetTickCount64();
    float dtScale = 1.0f; // relative to ~15ms WM_TIMER resolution (see Window.cpp comment)
    if (m_lastAutoScrollMs != 0) {
        float dtMs = static_cast<float>(nowMs - m_lastAutoScrollMs);
        dtScale = dtMs / 15.0f;
    }
    m_lastAutoScrollMs = nowMs;
    dtScale = std::clamp(dtScale, 0.25f, 2.0f);

    // Auto-scroll Y - use visible area's midline + hysteresis to avoid jitter.
    float contentTop = m_bounds.y + m_headerHeight;
    float contentBottom = m_bounds.y + m_bounds.height;
    float contentH = contentBottom - contentTop;
    if (contentH > 0.0f) {
        float midY = contentTop + contentH * 0.5f;
        float deltaY = mouseY - midY; // +: mouse below midline (scroll down), -: mouse above
        float absDeltaY = std::abs(deltaY);

        // Scale acceleration relative to list height so it doesn't saturate immediately.
        float speedDivisor = std::max(5.0f, contentH / 12.0f);

        // Hysteresis zone around midline (5% of visible height).
        // Increase hysteresis to reduce up/down jitter while dragging.
        float switchZone = std::max(2.0f, contentH * 0.15f);

        int dirY = m_autoScrollDirY;
        if (absDeltaY >= switchZone) {
            dirY = (deltaY >= 0.0f) ? 1 : -1;
            m_autoScrollDirY = dirY;
        }

        float speed = std::min(100.0f, 2.0f + std::pow(absDeltaY / speedDivisor, 2.0f));
        float scaledSpeed = std::min(100.0f, speed * dtScale);
        if (dirY > 0) m_scrollY += scaledSpeed;
        else m_scrollY -= scaledSpeed;
        scrolled = true;
    }

    // Auto-scroll X - keep existing edge-based behavior
    if (mouseX < m_bounds.x) {
        float overDistance = m_bounds.x - mouseX;
        float speed = std::min(100.0f, 8.0f + std::pow(overDistance / 5.0f, 2.0f));
        float scaledSpeed = std::min(100.0f, speed * dtScale);
        m_scrollX -= scaledSpeed;
        scrolled = true;
    } else if (mouseX > m_bounds.x + m_bounds.width) {
        float overDistance = mouseX - (m_bounds.x + m_bounds.width);
        float speed = std::min(100.0f, 8.0f + std::pow(overDistance / 5.0f, 2.0f));
        float scaledSpeed = std::min(100.0f, speed * dtScale);
        m_scrollX += scaledSpeed;
        scrolled = true;
    }

    return scrolled;
}

void ListView::OnAutoScrollTick() {
    if (!m_isRubberBandSelecting) return;

    if (ApplyAutoScroll()) {
        ClampScroll();
        UpdateRubberBandSelection();
    }
}

void ListView::OnKeyDown(int vkCode) {
    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    if (ctrlDown && vkCode == 'A') {
        SelectAll();
        return;
    }

    int rowCount = static_cast<int>(GetRowCount());
    if (rowCount == 0) return;

    int activeRow = (m_anchorIndex != -1) ? m_anchorIndex : 0;
    int newRow = activeRow;
    int visibleCount = static_cast<int>((m_bounds.height - m_headerHeight - 4.0f) / m_rowHeight);

    switch (vkCode) {
    case VK_UP:
        newRow = std::max(0, activeRow - 1);
        break;
    case VK_DOWN:
        newRow = std::min(rowCount - 1, activeRow + 1);
        break;
    case VK_PRIOR: // Page Up
        newRow = std::max(0, activeRow - visibleCount);
        break;
    case VK_NEXT: // Page Down
        newRow = std::min(rowCount - 1, activeRow + visibleCount);
        break;
    case VK_HOME:
        newRow = 0;
        break;
    case VK_END:
        newRow = rowCount - 1;
        break;
    }

    if (newRow != activeRow) {
        m_anchorIndex = newRow;
        m_selectedIndices.clear();
        m_selectedIndices.insert(newRow);
        m_onSelectionChangedEvent.Invoke(this, newRow);
    }
}

bool ListView::OnAnimationTick() {
    return AdvanceSmoothScroll();
}

} // namespace CUI
