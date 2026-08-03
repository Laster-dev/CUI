#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ListView.h"
#include "../style/ThemeManager.h"
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
    SetProperty("theme.backgroundToken", Value("windowBackground"));
    SetProperty("theme.headerBackgroundToken", Value("cardBackground"));
    SetProperty("theme.borderToken", Value("cardBorder"));
    SetProperty("theme.gridLineBrushToken", Value("inputBorder"));
    SetProperty("theme.colorToken", Value("textPrimary"));
    SetProperty("theme.selectedBackgroundToken", Value("selectedBackground"));
    SetProperty("theme.hoverBackgroundToken", Value("hoverBackground"));
    SetProperty("background", Value(ThemeManager::Instance().GetColor("windowBackground")));
    SetProperty("headerBackground", Value(ThemeManager::Instance().GetColor("cardBackground")));
    SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor("cardBorder")));
    SetProperty("gridLineBrush", Value(ThemeManager::Instance().GetColor("inputBorder")));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("color", Value(ThemeManager::Instance().GetColor("textPrimary")));
    SetProperty("selectedBackground", Value(ThemeManager::Instance().GetColor("selectedBackground")));
    SetProperty("hoverBackground", Value(ThemeManager::Instance().GetColor("hoverBackground")));
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
    m_targetScrollY = 0.0f;
    m_scrollYAnim.Reset(0.0f);
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
    m_targetScrollY = 0.0f;
    m_scrollYAnim.Reset(0.0f);
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
    m_targetScrollY = 0.0f;
    m_scrollYAnim.Reset(0.0f);
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
    m_targetScrollY = 0.0f;
    m_scrollYAnim.Reset(0.0f);
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
    m_maxScrollY = (std::max)(0.0f, totalContentH - viewH);
    m_targetScrollY = std::clamp(m_targetScrollY, 0.0f, m_maxScrollY);
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
    }

    float totalColsW = GetTotalColumnsWidth();
    float viewW = m_bounds.width - 4.0f;
    m_maxScrollX = (std::max)(0.0f, totalColsW - viewW);
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
    D2D1_COLOR_F bg = ResolveThemeColor("theme.backgroundToken", "windowBackground");
    D2D1_COLOR_F headerBg = ResolveThemeColor("theme.headerBackgroundToken", "cardBackground");
    D2D1_COLOR_F borderClr = ResolveThemeColor("theme.borderToken", "cardBorder");
    D2D1_COLOR_F gridLineClr = ResolveThemeColor("theme.gridLineBrushToken", "inputBorder");
    D2D1_COLOR_F textClr = ResolveThemeColor("theme.colorToken", "textPrimary");
    D2D1_COLOR_F selectedBg = ResolveThemeColor("theme.selectedBackgroundToken", "selectedBackground");
    D2D1_COLOR_F hoverBg = ResolveThemeColor("theme.hoverBackgroundToken", "hoverBackground");
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
        ctx.DrawText(col.header, colTextRect, ThemeManager::Instance().GetColor("textSecondary"), font, fontH, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

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
                D2D1_COLOR_F cellClr = isSelected ? ThemeManager::Instance().GetColor("textPrimary") : textClr;
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
            D2D1_COLOR_F accent = ThemeManager::Instance().GetColor("accentColor");
            ctx.FillRect(rubberRect, D2D1::ColorF(accent.r, accent.g, accent.b, 0.25f));
            ctx.DrawRect(rubberRect, accent, 1.0f);
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
        ctx.FillRoundedRect(thumbRect, 3.0f, D2D1::ColorF(borderClr.r, borderClr.g, borderClr.b, 0.6f));
    }

    // 5. Column Drag Reordering Indicator Card & Insertion Line
    if (m_isReorderingColumn && m_reorderingColumnIndex >= 0 && m_reorderingColumnIndex < static_cast<int>(m_columns.size())) {
        D2D1_COLOR_F accent = ThemeManager::Instance().GetColor("accentColor");
        
        float dropX = m_bounds.x - m_scrollX;
        float currX = m_bounds.x - m_scrollX;
        for (size_t c = 0; c < m_columns.size(); ++c) {
            float colMid = currX + m_columns[c].width * 0.5f;
            if (m_columnDragCurrentX < colMid) {
                dropX = currX;
                break;
            }
            currX += m_columns[c].width;
            dropX = currX;
        }

        // Draw vertical insertion line indicator across full height
        ctx.DrawLine(Point(dropX, m_bounds.y), Point(dropX, m_bounds.y + m_bounds.height), accent, 3.0f);

        // Draw floating drag card under cursor
        float dragColW = m_columns[m_reorderingColumnIndex].width;
        Rect dragCard(m_columnDragCurrentX - dragColW * 0.5f, m_bounds.y + 2.0f, dragColW, m_headerHeight - 4.0f);
        ctx.FillRoundedRect(dragCard, 4.0f, D2D1::ColorF(accent.r, accent.g, accent.b, 0.45f));
        ctx.DrawRoundedRect(dragCard, 4.0f, accent, 1.5f);
        ctx.DrawText(m_columns[m_reorderingColumnIndex].header, dragCard, D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), font, fontH, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
    }

    ctx.PopClip();
}

void ListView::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    m_targetScrollY = m_scrollY;
    m_scrollYAnim.Reset(m_scrollY);

    m_isMouseDown = true;
    m_mouseDownPoint = pt;
    m_initialSelectedBeforeDrag = m_selectedIndices;
    m_pendingRowClick = -1;
    m_isReorderingColumn = false;
    m_reorderingColumnIndex = -1;

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

    // 2. Check Header Bar: Splitter Resize OR Column Drag Reordering
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
            if (pt.x >= currX && pt.x < splitterX) {
                m_reorderingColumnIndex = static_cast<int>(c);
                m_columnDragStartX = pt.x;
                m_columnDragCurrentX = pt.x;
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
            m_targetScrollY = m_dragStartScrollY + (deltaY / scrollableTrackH) * m_maxScrollY;
            ClampScroll();
            m_scrollY = m_targetScrollY;
            m_scrollYAnim.Reset(m_scrollY);
            MarkRenderContentDirty();
        }
        return;
    }

    // 2. Column Resizing Drag
    if (m_isResizingColumn && m_resizingColumnIndex >= 0 && m_resizingColumnIndex < static_cast<int>(m_columns.size())) {
        float deltaX = pt.x - m_dragStartX;
        float newW = std::max(m_columns[m_resizingColumnIndex].minWidth, m_initialColumnWidth + deltaX);
        if (m_columns[m_resizingColumnIndex].width != newW) {
            m_columns[m_resizingColumnIndex].width = newW;
            ClampScroll();
            MarkRenderContentDirty();
        }
        return;
    }

    // 3. Column Reordering Drag
    if (m_isMouseDown && m_reorderingColumnIndex >= 0 && !m_isResizingColumn && !m_isDraggingScrollbar) {
        float dx = std::abs(pt.x - m_columnDragStartX);
        if (dx > 4.0f || m_isReorderingColumn) {
            m_isReorderingColumn = true;
            m_columnDragCurrentX = pt.x;
            MarkRenderContentDirty();
            return;
        }
    }

    // 4. Check if mouse drag exceeds threshold to start Rubber-Band Marquee Selection
    if (m_isMouseDown && !m_isResizingColumn && !m_isReorderingColumn && !m_isDraggingScrollbar && !m_isRubberBandSelecting) {
        float dx = pt.x - m_mouseDownPoint.x;
        float dy = pt.y - m_mouseDownPoint.y;
        if (std::sqrt(dx * dx + dy * dy) > 4.0f) {
            m_isRubberBandSelecting = true;
            m_rubberBandStart.x = m_mouseDownPoint.x - m_bounds.x + m_scrollX;
            m_rubberBandStart.y = m_mouseDownPoint.y - m_bounds.y - m_headerHeight + m_scrollY;
            m_rubberBandScrollOffsetY = m_scrollY;
        }
    }

    // 5. Rubber-Band Drag Selection
    if (m_isRubberBandSelecting) {
        m_rubberBandCurrent = pt;
        m_autoScrollLastMouseX = pt.x;
        m_autoScrollLastMouseY = pt.y;
        bool scrolled = ApplyAutoScroll();
        if (scrolled) {
            ClampScroll();
        }
        UpdateRubberBandSelection();
        MarkRenderContentDirty();
        return;
    }

    // 6. Hover state on Column Splitter lines
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

    // Column Reordering Drop Execution
    if (m_isReorderingColumn && m_reorderingColumnIndex >= 0 && m_reorderingColumnIndex < static_cast<int>(m_columns.size())) {
        int dropIndex = -1;
        float currX = m_bounds.x - m_scrollX;
        for (size_t c = 0; c < m_columns.size(); ++c) {
            float colMid = currX + m_columns[c].width * 0.5f;
            if (pt.x < colMid) {
                dropIndex = static_cast<int>(c);
                break;
            }
            currX += m_columns[c].width;
        }
        if (dropIndex == -1) {
            dropIndex = static_cast<int>(m_columns.size()) - 1;
        }

        int srcIndex = m_reorderingColumnIndex;
        if (dropIndex != srcIndex && dropIndex >= 0 && dropIndex < static_cast<int>(m_columns.size())) {
            ListViewColumn col = m_columns[srcIndex];
            m_columns.erase(m_columns.begin() + srcIndex);
            m_columns.insert(m_columns.begin() + dropIndex, col);

            if (!m_virtualMode) {
                for (auto& row : m_rows) {
                    if (srcIndex < static_cast<int>(row.size())) {
                        ListViewCellData cell = row[srcIndex];
                        row.erase(row.begin() + srcIndex);
                        if (dropIndex < static_cast<int>(row.size())) {
                            row.insert(row.begin() + dropIndex, cell);
                        } else {
                            row.push_back(cell);
                        }
                    }
                }
            }
            MarkRenderContentDirty();
        }
    }

    int rowCount = static_cast<int>(GetRowCount());

    // If mouse was clicked without dragging, perform normal row selection!
    if (!m_isRubberBandSelecting && !m_isResizingColumn && !m_isReorderingColumn && m_pendingRowClick != -1) {
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
    } else if (!m_isRubberBandSelecting && !m_isResizingColumn && !m_isReorderingColumn && m_pendingRowClick == -1) {
        // Clicked empty area without dragging -> clear selection
        m_selectedIndices.clear();
        m_anchorIndex = -1;
        m_onSelectionChangedEvent.Invoke(this, -1);
    }

    m_isMouseDown = false;
    m_isResizingColumn = false;
    m_isReorderingColumn = false;
    m_reorderingColumnIndex = -1;
    m_isRubberBandSelecting = false;
    m_isDraggingScrollbar = false;
    m_pendingRowClick = -1;
}

void ListView::OnMouseWheel(float delta) {
    bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    if (shiftDown) {
        m_scrollX -= delta * 40.0f;
        ClampScroll();
        MarkRenderContentDirty();
    } else {
        float scrollStep = m_rowHeight * 2.5f;
        m_targetScrollY -= delta * scrollStep;
        ClampScroll();
        m_scrollYAnim.SetTarget(m_targetScrollY);
        if (!UIElement::AreAnimationsEnabled()) {
            m_scrollY = m_targetScrollY;
            m_scrollYAnim.Reset(m_scrollY);
            MarkRenderContentDirty();
        }
    }
}

bool ListView::ApplyAutoScroll() {
    float mouseX = m_autoScrollLastMouseX;
    float mouseY = m_autoScrollLastMouseY;
    bool scrolled = false;

    float contentTop = m_bounds.y + m_headerHeight;
    float contentBottom = m_bounds.y + m_bounds.height;

    // Auto-scroll Y ONLY when mouse is dragged outside top or bottom edges!
    if (mouseY < contentTop) {
        float dist = contentTop - mouseY;
        float speed = std::min(120.0f, 4.0f + std::pow(dist / 4.0f, 1.8f));
        m_targetScrollY = (std::max)(0.0f, m_targetScrollY - speed);
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
        scrolled = true;
    } else if (mouseY > contentBottom) {
        float dist = mouseY - contentBottom;
        float speed = std::min(120.0f, 4.0f + std::pow(dist / 4.0f, 1.8f));
        m_targetScrollY = (std::min)(m_maxScrollY, m_targetScrollY + speed);
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
        scrolled = true;
    }

    // Auto-scroll X ONLY when mouse is dragged outside left or right edges!
    if (mouseX < m_bounds.x) {
        float dist = m_bounds.x - mouseX;
        float speed = std::min(120.0f, 4.0f + std::pow(dist / 4.0f, 1.8f));
        m_scrollX = (std::max)(0.0f, m_scrollX - speed);
        scrolled = true;
    } else if (mouseX > m_bounds.x + m_bounds.width) {
        float dist = mouseX - (m_bounds.x + m_bounds.width);
        float speed = std::min(120.0f, 4.0f + std::pow(dist / 4.0f, 1.8f));
        m_scrollX = (std::min)(m_maxScrollX, m_scrollX + speed);
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
    bool base = Control::OnAnimationTick();
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
        return base;
    }
    float dt = UIElement::GetAnimationDeltaSeconds();
    m_scrollYAnim.SetTarget(m_targetScrollY);
    bool anim = m_scrollYAnim.Tick(dt, AnimationSpec{ 0.55f, 0.01f });
    if (anim) {
        m_scrollY = m_scrollYAnim.Current();
        MarkRenderContentDirty();
    }
    return base || anim;
}

bool ListView::HasSelfAnimation() const {
    return Control::HasSelfAnimation() ||
           std::abs(m_scrollYAnim.Target() - m_scrollYAnim.Current()) > 0.001f;
}

} // namespace CUI
