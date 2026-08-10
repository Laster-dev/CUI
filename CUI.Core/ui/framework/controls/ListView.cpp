#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ListView.h"
#include "ContextMenu.h"
#include "../style/ThemeManager.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <windows.h>

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

DWRITE_FONT_WEIGHT ResolveListFontWeight(const std::string& weight) {
    if (weight == "Bold" || weight == "bold") return DWRITE_FONT_WEIGHT_BOLD;
    if (weight == "SemiBold" || weight == "semibold" || weight == "Medium" || weight == "medium") {
        return DWRITE_FONT_WEIGHT_SEMI_BOLD;
    }
    if (weight == "Light" || weight == "light") return DWRITE_FONT_WEIGHT_LIGHT;
    return DWRITE_FONT_WEIGHT_NORMAL;
}

float FrameBlend(float factorAt60Hz) {
    factorAt60Hz = std::clamp(factorAt60Hz, 0.0f, 0.999f);
    float frames = UIElement::GetAnimationDeltaSeconds() * 60.0f;
    return 1.0f - std::pow(1.0f - factorAt60Hz, (std::max)(0.1f, frames));
}

constexpr float kPillInsetX = 4.0f;
constexpr float kPillInsetY = 2.0f;
constexpr float kPillRadius = 4.0f;
// Keep scrollbar clear of the window's ~8px NC resize border on the right edge.
constexpr float kScrollbarThumbW = 6.0f;
constexpr float kScrollbarEdgeGap = 12.0f;
constexpr float kScrollbarHitPad = 2.0f;

float ScrollbarTrackX(const Rect& bounds) {
    return bounds.x + bounds.width - kScrollbarEdgeGap - kScrollbarThumbW;
}

bool IsOverScrollbar(const Rect& bounds, float headerHeight, Point pt, bool canScroll) {
    if (!canScroll) return false;
    const float x0 = ScrollbarTrackX(bounds) - kScrollbarHitPad;
    const float x1 = ScrollbarTrackX(bounds) + kScrollbarThumbW + kScrollbarHitPad;
    return pt.x >= x0 && pt.x < x1 && pt.y >= bounds.y + headerHeight;
}

} // namespace

ListView::ListView() {
    // 只绑 token；绘制走 ThemeManager。列表表面用 card，勿用 window（材质模式下 window 全透明）。
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetHeaderBackgroundToken(ThemeTokenId::PaneBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetGridLineBrushToken(ThemeTokenId::InputBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
    SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    SetBorderThickness(1.0f);
    SetFontSize(16.0f);
    SetFontFamily("微软雅黑");
    SetFontWeight("Normal");
    SetCornerRadius(4.0f);
    // No fixed size — let parent layout stretch ListView to the pane.
    SetWidth(-1.0f);
    SetHeight(-1.0f);
    SetRowHeight(30.0f);
    m_rowsLayer.SetCacheable(true);
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
    m_rowIcons.clear();
    for (const auto& row : rowsData) {
        AddRow(row);
    }
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_caretIndex = -1;
    m_scrollY = 0.0f;
    m_targetScrollY = 0.0f;
    m_scrollYAnim.Reset(0.0f);
    InvalidateRowsLayer();
}

void ListView::SetRows(const std::vector<std::vector<ListViewCellData>>& rowsData) {
    ClearChildren();
    m_rows.clear();
    m_rowIcons.clear();
    for (const auto& row : rowsData) {
        AddRow(row);
    }
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_caretIndex = -1;
    m_scrollY = 0.0f;
    m_targetScrollY = 0.0f;
    m_scrollYAnim.Reset(0.0f);
    InvalidateRowsLayer();
}

void ListView::ClearRows() {
    ClearChildren();
    m_rows.clear();
    m_rowIcons.clear();
    m_virtualMode = false;
    m_virtualRowCount = 0;
    m_dataSource = nullptr;
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_caretIndex = -1;
    m_scrollY = 0.0f;
    m_targetScrollY = 0.0f;
    m_scrollYAnim.Reset(0.0f);
    InvalidateRowsLayer();
}

void ListView::SetRowIcons(const std::vector<HICON>& icons) {
    m_rowIcons = icons;
    InvalidateRowsLayer();
}

void ListView::ClearRowIcons() {
    m_rowIcons.clear();
    InvalidateRowsLayer();
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
    m_caretIndex = -1;
    m_scrollY = 0.0f;
    m_targetScrollY = 0.0f;
    m_scrollYAnim.Reset(0.0f);
    InvalidateRowsLayer();
}

void ListView::SetVirtualRowCount(int rowCount) {
    if (!m_virtualMode || m_dataSource == nullptr) {
        SetVirtualMode(rowCount, m_dataSource);
        return;
    }
    if (m_virtualRowCount == rowCount) return;
    m_virtualRowCount = rowCount;
    ClampScroll();
    InvalidateRowsLayer();
    MarkRenderContentDirty();
}

void ListView::RefreshRows() {
    InvalidateRowsLayer();
    MarkRenderContentDirty();
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
    if (m_selectionMode == ListViewSelectionMode::Single) return;
    m_selectedIndices.clear();
    int rowCount = static_cast<int>(GetRowCount());
    for (int i = 0; i < rowCount; ++i) {
        m_selectedIndices.insert(i);
    }
    m_onSelectionChangedEvent.Invoke(this, -1);
    InvalidateRowsLayer();
}

void ListView::ClearSelection() {
    m_selectedIndices.clear();
    m_anchorIndex = -1;
    m_caretIndex = -1;
    m_onSelectionChangedEvent.Invoke(this, -1);
    InvalidateRowsLayer();
}

void ListView::SetRowSelected(int rowIndex, bool selected) {
    if (rowIndex >= 0 && rowIndex < static_cast<int>(GetRowCount())) {
        if (selected) {
            if (m_selectionMode == ListViewSelectionMode::Single) {
                m_selectedIndices.clear();
            }
            m_selectedIndices.insert(rowIndex);
            m_caretIndex = rowIndex;
            m_anchorIndex = rowIndex;
        } else {
            m_selectedIndices.erase(rowIndex);
        }
        m_onSelectionChangedEvent.Invoke(this, rowIndex);
        InvalidateRowsLayer();
    }
}

bool ListView::IsRowSelected(int rowIndex) const {
    return m_selectedIndices.find(rowIndex) != m_selectedIndices.end();
}

void ListView::SetCaretIndex(int index) {
    int rowCount = static_cast<int>(GetRowCount());
    if (index >= 0 && index < rowCount) {
        m_caretIndex = index;
        EnsureVisible(index);
        InvalidateRowsLayer();
    }
}

void ListView::EnsureVisible(int rowIndex) {
    int rowCount = static_cast<int>(GetRowCount());
    if (rowIndex < 0 || rowIndex >= rowCount) return;

    float rowTop = rowIndex * m_rowHeight;
    float rowBottom = rowTop + m_rowHeight;
    float viewH = m_bounds.height - m_headerHeight - 4.0f;

    if (rowTop < m_targetScrollY) {
        m_targetScrollY = rowTop;
    } else if (rowBottom > m_targetScrollY + viewH) {
        m_targetScrollY = rowBottom - viewH;
    }
    ClampScroll();
    m_scrollYAnim.SetTarget(m_targetScrollY);
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
    } else {
        RequestAnimationTicks();
    }
    MarkRenderRectDirty(m_bounds);
}

void ListView::SelectRange(int fromIdx, int toIdx, bool keepExisting) {
    int rowCount = static_cast<int>(GetRowCount());
    if (rowCount == 0) return;

    fromIdx = std::clamp(fromIdx, 0, rowCount - 1);
    toIdx = std::clamp(toIdx, 0, rowCount - 1);

    if (!keepExisting) {
        m_selectedIndices.clear();
    }

    int start = (std::min)(fromIdx, toIdx);
    int end = (std::max)(fromIdx, toIdx);

    for (int r = start; r <= end; ++r) {
        m_selectedIndices.insert(r);
    }
    m_caretIndex = toIdx;
    EnsureVisible(toIdx);
    m_onSelectionChangedEvent.Invoke(this, toIdx);
    InvalidateRowsLayer();
}

float ListView::GetColumnWidth(size_t index) const {
    if (index >= m_columns.size() || !m_columns[index].visible) return 0.0f;
    const float base = m_columns[index].width;
    // Last visible column absorbs leftover viewport width so the list fills the pane.
    size_t lastVisible = m_columns.size();
    while (lastVisible > 0 && !m_columns[lastVisible - 1].visible) --lastVisible;
    if (lastVisible == 0) return 0.0f;
    if (index + 1 == lastVisible && m_bounds.width > 0.0f) {
        float others = 0.0f;
        for (size_t i = 0; i + 1 < m_columns.size(); ++i) {
            if (m_columns[i].visible) others += m_columns[i].width;
        }
        const float avail = (std::max)(0.0f, m_bounds.width - 2.0f);
        return (std::max)(base, avail - others);
    }
    return base;
}

void ListView::SetColumnVisible(int columnIndex, bool visible) {
    if (columnIndex < 0 || columnIndex >= static_cast<int>(m_columns.size())) return;
    m_columns[static_cast<size_t>(columnIndex)].visible = visible;
    ClampScroll();
    InvalidateRowsLayer();
    MarkRenderContentDirty();
}

bool ListView::IsColumnVisible(int columnIndex) const {
    if (columnIndex < 0 || columnIndex >= static_cast<int>(m_columns.size())) return false;
    return m_columns[static_cast<size_t>(columnIndex)].visible;
}

int ListView::HitTestHeaderColumn(float x) const {
    if (x < m_bounds.x || x > m_bounds.x + m_bounds.width) return -1;
    float currX = m_bounds.x - m_scrollX;
    for (size_t c = 0; c < m_columns.size(); ++c) {
        if (!m_columns[c].visible) continue;
        const float colW = GetColumnWidth(c);
        if (x >= currX && x < currX + colW) return static_cast<int>(c);
        currX += colW;
    }
    return -1;
}

void ListView::RebuildHeaderContextMenu() {
    m_headerContextMenu = std::make_shared<ContextMenu>();
    for (size_t i = 0; i < m_columns.size(); ++i) {
        const int col = static_cast<int>(i);
        auto item = m_headerContextMenu->AddItem(m_columns[i].header, [this, col]() {
            SetColumnVisible(col, !IsColumnVisible(col));
        });
        item->SetChecked(m_columns[i].visible);
    }
}

bool ListView::OnContextMenuRelease(Point pt) {
    if (m_headerContextMenuPending && m_headerContextMenu) {
        m_headerContextMenuPending = false;
        m_headerContextMenu->ShowAt(pt.x, pt.y);
        return true;
    }
    if (m_shellContextMenuHandler) {
        std::vector<int> rows;
        for (int r : m_selectedIndices) rows.push_back(r);
        if (rows.empty() && m_caretIndex >= 0) rows.push_back(m_caretIndex);
        return m_shellContextMenuHandler(pt, rows);
    }
    return false;
}

float ListView::GetTotalColumnsWidth() const {
    if (m_columns.empty()) return 0.0f;
    float totalW = 0.0f;
    for (size_t i = 0; i < m_columns.size(); ++i) {
        totalW += GetColumnWidth(i);
    }
    return totalW;
}

Size ListView::Measure(Size availableSize) {
    float expW = GetWidth();
    float expH = GetHeight();
    // Prefer available space so Flex/Column parents can stretch the control.
    if (expW < 0.0f) {
        expW = (availableSize.width > 0.0f && availableSize.width < 1.0e6f)
                   ? availableSize.width
                   : 480.0f;
    }
    if (expH < 0.0f) {
        expH = (availableSize.height > 0.0f && availableSize.height < 1.0e6f)
                   ? availableSize.height
                   : 320.0f;
    }
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void ListView::ClampScroll() {
    float totalContentH = m_rowHeight * static_cast<float>(GetRowCount());
    float viewH = (std::max)(0.0f, m_bounds.height - m_headerHeight - 4.0f);
    m_maxScrollY = (std::max)(0.0f, totalContentH - viewH);
    m_targetScrollY = std::clamp(m_targetScrollY, 0.0f, m_maxScrollY);
    m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScrollY);
    if (!UIElement::AreAnimationsEnabled() || m_maxScrollY == 0.0f) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
    }

    float totalColsW = GetTotalColumnsWidth();
    float viewW = (std::max)(0.0f, m_bounds.width - 4.0f);
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

    float currentContentX = m_rubberBandCurrent.x - m_bounds.x + m_scrollX;
    float currentContentY = m_rubberBandCurrent.y - m_bounds.y - m_headerHeight + m_scrollY;

    float minContentX = std::min(m_rubberBandStart.x, currentContentX);
    float maxContentX = std::max(m_rubberBandStart.x, currentContentX);
    float minContentY = std::min(m_rubberBandStart.y, currentContentY);
    float maxContentY = std::max(m_rubberBandStart.y, currentContentY);

    int rowCount = static_cast<int>(GetRowCount());
    if (rowCount == 0) return;

    int startRow = std::max(0, static_cast<int>(std::floor(minContentY / m_rowHeight)));
    int endRow = std::min(rowCount - 1, static_cast<int>(std::ceil(maxContentY / m_rowHeight)) - 1);

    if (maxContentY <= minContentY || startRow > endRow || maxContentY <= 0.0f || minContentY >= (rowCount * m_rowHeight)) {
        startRow = -1;
        endRow = -1;
    }

    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    std::unordered_set<int> newSelection;

    if (ctrlDown) {
        newSelection = m_initialSelectedBeforeDrag;
        if (startRow != -1 && endRow != -1) {
            for (int r = startRow; r <= endRow; ++r) {
                if (m_initialSelectedBeforeDrag.count(r)) {
                    newSelection.erase(r);
                } else {
                    newSelection.insert(r);
                }
            }
        }
    } else {
        if (startRow != -1 && endRow != -1) {
            for (int r = startRow; r <= endRow; ++r) {
                newSelection.insert(r);
            }
        }
    }

    if (m_selectedIndices != newSelection) {
        m_selectedIndices = newSelection;
        m_onSelectionChangedEvent.Invoke(this, -1);
    }
}

void ListView::Render(GraphicsContext& ctx) {
    if (GetVisibility() != Visibility::Visible) return;

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
    if (GetVisibility() != Visibility::Visible) return nullptr;

    if (m_bounds.Contains(x, y)) {
        int r = GetRowIndexFromY(y);
        int rowCount = static_cast<int>(GetRowCount());
        if (r >= 0 && r < rowCount) {
            float rowY = m_bounds.y + m_headerHeight + r * m_rowHeight - m_scrollY;
            float cellX = m_bounds.x - m_scrollX;

            for (size_t c = 0; c < m_columns.size(); ++c) {
                float colW = GetColumnWidth(c);
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

void ListView::InvalidateRowsLayer() {
    m_rowsLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::StructureDirty);
    m_rowsLayerCachesFull = false;
    MarkRenderRectDirty(m_bounds);
}

void ListView::OnThemeChanged() {
    UIElement::OnThemeChanged();
    InvalidateRowsLayer();
}

bool ListView::CanCacheFullRows() const {
    const float contentH = GetRowsContentHeight();
    const float contentW = (std::max)(m_bounds.width, GetTotalColumnsWidth());
    return contentH > 0.0f && contentH <= kMaxFullContentCacheHeight
        && contentW > 0.0f && contentW <= kMaxFullContentCacheWidth;
}

float ListView::GetRowsContentHeight() const {
    return m_rowHeight * static_cast<float>(GetRowCount());
}

Rect ListView::GetRowsViewportRect() const {
    return Rect(
        m_bounds.x + 1.0f,
        m_bounds.y + m_headerHeight + 1.0f,
        (std::max)(0.0f, m_bounds.width - 2.0f),
        (std::max)(0.0f, m_bounds.height - m_headerHeight - 2.0f));
}

void ListView::PaintRowsRange(GraphicsContext& ctx, int startRow, int endRow, float scrollX, float scrollY) {
    D2D1_COLOR_F gridLineClr = ResolveThemeColor(GetGridLineBrushToken(), ThemeTokenId::InputBorder);
    D2D1_COLOR_F textClr = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    D2D1_COLOR_F selectedBg = ResolveThemeColor(GetSelectedBackgroundToken(), ThemeTokenId::SelectedBackground);
    D2D1_COLOR_F hoverBg = ResolveThemeColor(GetHoverBackgroundToken(), ThemeTokenId::HoverBackground);
    D2D1_COLOR_F focusBorderColor = ResolveThemeColor(GetBorderToken(), ThemeTokenId::AccentColor);
    std::string font = GetFontFamily();
    float fontH = GetFontSize();
    const DWRITE_FONT_WEIGHT weight = ResolveListFontWeight(GetFontWeight());
    float totalColsW = GetTotalColumnsWidth();
    bool isFocused = m_isFocused;

    // Pass 1: inset rounded selection/hover pills — selection reveal uses a
    // Button-style expanding ripple clipped to the pill until it covers.
    for (int r = startRow; r <= endRow; ++r) {
        float rowY = m_bounds.y + m_headerHeight + r * m_rowHeight - scrollY;
        Rect rowRect(m_bounds.x, rowY, std::max(m_bounds.width, totalColsW), m_rowHeight);

        bool isSelected = IsRowSelected(r);
        bool isHovered = (r == m_hoveredRowIndex);
        bool isCaret = (r == m_caretIndex);

        const float pillW = (std::max)(0.0f, (std::min)(rowRect.width, m_bounds.width) - kPillInsetX * 2.0f);
        const float pillH = (std::max)(0.0f, rowRect.height - kPillInsetY * 2.0f);
        Rect pill(rowRect.x + kPillInsetX, rowRect.y + kPillInsetY, pillW, pillH);

        const bool rippleHere = m_selectRippleActive && r == m_selectRippleRow;
        const bool rippleCovering = rippleHere && !m_selectRippleCovered;

        if (isSelected) {
            if (rippleCovering) {
                // Same selectedBg as the final fill — only geometry expands (no tint overlay).
                ctx.PushRoundedClip(pill, kPillRadius);
                const float cx = pill.x + m_selectRippleLocalX;
                const float cy = pill.y + m_selectRippleLocalY;
                Rect rippleRect(
                    cx - m_selectRippleRadius,
                    cy - m_selectRippleRadius,
                    m_selectRippleRadius * 2.0f,
                    m_selectRippleRadius * 2.0f);
                ctx.FillRoundedRect(rippleRect, m_selectRippleRadius, selectedBg);
                ctx.PopClip();
            } else {
                ctx.FillRoundedRect(pill, kPillRadius, selectedBg);
            }
        } else if (isHovered && IsEnabled()) {
            ctx.FillRoundedRect(pill, kPillRadius, hoverBg);
        }

        if (isCaret && isFocused) {
            D2D1_COLOR_F ring = focusBorderColor;
            ring.a = (std::min)(1.0f, ring.a * 0.65f + 0.25f);
            ctx.DrawRoundedRect(pill, kPillRadius, ring, 1.0f);
        }
    }

    // Pass 2: grid lines after all fills so separators stay visible on hover/selection.
    if (m_showGridLines) {
        for (int r = startRow; r <= endRow; ++r) {
            float rowY = m_bounds.y + m_headerHeight + r * m_rowHeight - scrollY;
            ctx.DrawLine(Point(m_bounds.x, rowY + m_rowHeight), Point(m_bounds.x + m_bounds.width, rowY + m_rowHeight), gridLineClr, 1.0f);
            float cellX = m_bounds.x - scrollX;
            for (size_t c = 0; c < m_columns.size(); ++c) {
                if (!m_columns[c].visible) continue;
                float colW = GetColumnWidth(c);
                if (colW <= 0.0f) continue;
                ctx.DrawLine(Point(cellX + colW, rowY), Point(cellX + colW, rowY + m_rowHeight), gridLineClr, 1.0f);
                cellX += colW;
            }
        }
    }

    // Pass 3: cell content above the grid.
    for (int r = startRow; r <= endRow; ++r) {
        float rowY = m_bounds.y + m_headerHeight + r * m_rowHeight - scrollY;
        bool isSelected = IsRowSelected(r);
        float cellX = m_bounds.x - scrollX;
        for (size_t c = 0; c < m_columns.size(); ++c) {
            if (!m_columns[c].visible) continue;
            float colW = GetColumnWidth(c);
            if (colW <= 0.0f) continue;
            auto cellElem = GetCellElement(r, static_cast<int>(c));
            if (cellElem) {
                Rect cellRect(cellX + 2.0f, rowY + 2.0f, colW - 4.0f, m_rowHeight - 4.0f);
                cellElem->Measure(Size(cellRect.width, cellRect.height));
                cellElem->Arrange(cellRect);
                cellElem->Render(ctx);
            } else {
                std::string cellText = GetCellText(r, static_cast<int>(c));
                float textPad = 8.0f;
                HICON rowIcon = nullptr;
                if (c == 0) {
                    if (m_virtualMode && m_dataSource) {
                        rowIcon = m_dataSource->GetRowIcon(r);
                    } else if (r >= 0 && r < static_cast<int>(m_rowIcons.size())) {
                        rowIcon = m_rowIcons[static_cast<size_t>(r)];
                    }
                }
                if (rowIcon) {
                    const float iconSize = 16.0f;
                    Rect iconRect(cellX + 6.0f, rowY + (m_rowHeight - iconSize) * 0.5f, iconSize, iconSize);
                    ctx.DrawHIcon(rowIcon, iconRect);
                    textPad = 6.0f + iconSize + 6.0f;
                }
                Rect cellRect(cellX + textPad, rowY, (std::max)(0.0f, colW - textPad - 8.0f), m_rowHeight);
                D2D1_COLOR_F cellClr = isSelected ? ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary) : textClr;
                // Clip + ellipsis so long names cannot bleed into the next column.
                ctx.PushClip(cellRect);
                ctx.DrawText(cellText, cellRect, cellClr, font, fontH, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, weight, true);
                ctx.PopClip();
            }
            cellX += colW;
        }
    }
}

void ListView::RenderRowsLayer(GraphicsContext& ctx) {
    const float contentH = GetRowsContentHeight();
    const float contentW = (std::max)(m_bounds.width, GetTotalColumnsWidth());
    const Rect viewport = GetRowsViewportRect();
    const float viewH = (std::min)(viewport.height, contentH);
    const float viewW = viewport.width;
    if (contentH <= 0.0f || contentW <= 0.0f || viewH <= 0.0f || viewW <= 0.0f) {
        return;
    }

    const Size cacheSize(contentW, contentH);
    const bool sizeChanged =
        std::abs(m_rowsLayer.GetCacheSurfaceSize().width - cacheSize.width) > 0.5f
        || std::abs(m_rowsLayer.GetCacheSurfaceSize().height - cacheSize.height) > 0.5f;
    const bool needsRerender = sizeChanged
        || !m_rowsLayerCachesFull
        || !m_rowsLayer.IsValid()
        || m_rowsLayer.NeedsContentRaster()
        || !m_rowsLayer.GetCacheBitmap();

    if (needsRerender) {
        Rect contentWorld(m_bounds.x, m_bounds.y + m_headerHeight, contentW, contentH);
        // Opaque clear: ClearType (and any leftover AA) needs a real background.
        // Transparent clears + text AA produced vertical fringe garbage in rows.
        D2D1_COLOR_F clearBg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
        clearBg.a = 1.0f;
        if (ctx.PushLayerTarget(m_rowsLayer, cacheSize, contentWorld, clearBg)) {
            auto* d2d = ctx.GetD2DContext();
            D2D1_MATRIX_3X2_F oldTransform{};
            d2d->GetTransform(&oldTransform);
            d2d->SetTransform(D2D1::Matrix3x2F::Translation(-contentWorld.x, -contentWorld.y));
            PaintRowsRange(ctx, 0, static_cast<int>(GetRowCount()) - 1, 0.0f, 0.0f);
            d2d->SetTransform(oldTransform);
            ctx.PopLayerTarget(m_rowsLayer);
            m_rowsLayer.Validate();
            m_rowsLayerCachesFull = true;
        }
    }

    const float srcX = std::clamp(m_scrollX, 0.0f, (std::max)(0.0f, contentW - viewW));
    const float srcY = std::clamp(m_scrollY, 0.0f, (std::max)(0.0f, contentH - viewH));
    Rect sourceRect(srcX, srcY, viewW, viewH);
    // Dest must match source DIP size — using the full viewport stretches few
    // rows to fill the pane (vertically elongated text / "one row fills page").
    Rect dest(viewport.x, viewport.y, viewW, viewH);
    ctx.PushClip(viewport);
    ctx.DrawLayer(m_rowsLayer, dest, &sourceRect);
    ctx.PopClip();
    m_rowsLayer.SetTranslation(-m_scrollX, -m_scrollY);
}

void ListView::OnRender(GraphicsContext& ctx) {
    ClampScroll();

    float radius = GetCornerRadius();
    (void)radius;
    D2D1_COLOR_F bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    D2D1_COLOR_F headerBg = ResolveThemeColor(GetHeaderBackgroundToken(), ThemeTokenId::PaneBackground);
    D2D1_COLOR_F borderClr = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    std::string font = GetFontFamily();
    float fontH = GetFontSize();
    const DWRITE_FONT_WEIGHT weight = ResolveListFontWeight(GetFontWeight());

    ctx.FillRect(m_bounds, bg);
    ctx.DrawRect(m_bounds, borderClr, 1.0f);

    Rect headerRect(m_bounds.x, m_bounds.y, m_bounds.width, m_headerHeight);
    ctx.FillRect(headerRect, headerBg);
    ctx.DrawLine(Point(m_bounds.x, m_bounds.y + m_headerHeight), Point(m_bounds.x + m_bounds.width, m_bounds.y + m_headerHeight), borderClr, 1.0f);

    float currColX = m_bounds.x - m_scrollX;
    ctx.PushClip(Rect(m_bounds.x + 1, m_bounds.y + 1, m_bounds.width - 2, m_headerHeight - 1));
    for (size_t colIdx = 0; colIdx < m_columns.size(); ++colIdx) {
        if (!m_columns[colIdx].visible) continue;
        const auto& col = m_columns[colIdx];
        const float colW = GetColumnWidth(colIdx);
        Rect colHeaderRect(currColX, m_bounds.y, colW, m_headerHeight);
        Rect colTextRect(colHeaderRect.x + 8.0f, colHeaderRect.y, colHeaderRect.width - 16.0f, colHeaderRect.height);
        std::string headerText = col.header;
        if (static_cast<int>(colIdx) == m_sortColumn) {
            headerText += m_sortAscending ? " ▲" : " ▼";
        }
        ctx.DrawText(headerText, colTextRect, ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary), font, fontH, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, weight, true);
        ctx.DrawLine(Point(currColX + colW, m_bounds.y + 4.0f), Point(currColX + colW, m_bounds.y + m_headerHeight - 4.0f), borderClr, 1.0f);
        currColX += colW;
    }
    ctx.PopClip();

    Rect contentArea = GetRowsViewportRect();
    int rowCount = static_cast<int>(GetRowCount());
    if (rowCount > 0 && CanCacheFullRows()) {
        RenderRowsLayer(ctx);
    } else if (rowCount > 0) {
        ctx.PushClip(contentArea);
        int startRow = std::max(0, static_cast<int>(m_scrollY / m_rowHeight));
        int endRow = std::min(rowCount - 1,
            static_cast<int>((m_scrollY + contentArea.height) / m_rowHeight) + 1);
        PaintRowsRange(ctx, startRow, endRow, m_scrollX, m_scrollY);
        ctx.PopClip();
    }

    ctx.PushClip(contentArea);
    if (m_isRubberBandSelecting) {
        float curContentX = m_rubberBandCurrent.x - m_bounds.x + m_scrollX;
        float curContentY = m_rubberBandCurrent.y - m_bounds.y - m_headerHeight + m_scrollY;

        float visTop = m_scrollY;
        float visBottom = m_scrollY + m_bounds.height - m_headerHeight;

        float drawTop = std::clamp(std::min(m_rubberBandStart.y, curContentY), visTop, visBottom);
        float drawBottom = std::clamp(std::max(m_rubberBandStart.y, curContentY), visTop, visBottom);

        float screenTop = m_bounds.y + m_headerHeight + drawTop - m_scrollY;
        float screenBottom = m_bounds.y + m_headerHeight + drawBottom - m_scrollY;

        float drawLeft = std::clamp(std::min(m_rubberBandStart.x, curContentX), 0.0f, GetTotalColumnsWidth());
        float drawRight = std::clamp(std::max(m_rubberBandStart.x, curContentX), 0.0f, GetTotalColumnsWidth());
        float screenLeft = m_bounds.x + drawLeft - m_scrollX;
        float screenRight = m_bounds.x + drawRight - m_scrollX;

        if (screenBottom > screenTop && screenRight > screenLeft) {
            Rect rubberRect(screenLeft, screenTop, screenRight - screenLeft, screenBottom - screenTop);
            bool dark = ThemeManager::Instance().GetThemeMode() == ThemeMode::Dark;
            D2D1_COLOR_F fillClr = dark ? D2D1::ColorF(0.0f, 0.60f, 1.0f, 0.15f) : D2D1::ColorF(0.0f, 0.45f, 0.90f, 0.12f);
            D2D1_COLOR_F strokeClr = dark ? D2D1::ColorF(0.35f, 0.82f, 1.0f, 0.95f) : D2D1::ColorF(0.0f, 0.45f, 0.90f, 0.95f);
            ctx.FillRect(rubberRect, fillClr);
            ctx.DrawRect(rubberRect, strokeClr, 1.5f);
        }
    }

    if (m_maxScrollY > 0.0f) {
        float trackX = ScrollbarTrackX(m_bounds);
        float trackY = m_bounds.y + m_headerHeight + 2.0f;
        float trackH = m_bounds.height - m_headerHeight - 4.0f;

        float contentH = m_rowHeight * static_cast<float>(GetRowCount());
        float thumbH = std::max(20.0f, trackH * (trackH / contentH));
        float thumbY = trackY + (m_scrollY / m_maxScrollY) * (trackH - thumbH);

        Rect thumbRect(trackX, thumbY, kScrollbarThumbW, thumbH);
        // Everything-style: always show thumb when scrollable (min 55% opacity).
        const float vis = (std::max)(m_scrollbarAutoHide.Opacity(), 0.55f);
        ctx.FillRoundedRect(thumbRect, 3.0f, D2D1::ColorF(borderClr.r, borderClr.g, borderClr.b, 0.75f * vis));
    }

    if (m_isReorderingColumn && m_reorderingColumnIndex >= 0 && m_reorderingColumnIndex < static_cast<int>(m_columns.size())) {
        D2D1_COLOR_F accent = ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor);

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

        ctx.DrawLine(Point(dropX, m_bounds.y), Point(dropX, m_bounds.y + m_bounds.height), accent, 3.0f);

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
    m_isReorderingColumn = false;
    m_reorderingColumnIndex = -1;

    // 1. Check Vertical ScrollBar Track / Thumb Click
    if (IsOverScrollbar(m_bounds, m_headerHeight, pt, m_maxScrollY > 0.0f)) {
        m_isDraggingScrollbar = true;
        m_scrollbarAutoHide.SetDragging(true, this);
        m_scrollbarAutoHide.NotifyActivity(this);
        RequestAnimationTicks();
        m_dragStartY = pt.y;
        m_dragStartScrollY = m_scrollY;
        return;
    }

    // 2. Check Header Bar: Splitter Resize OR Column Drag Reordering
    if (pt.y >= m_bounds.y && pt.y <= m_bounds.y + m_headerHeight) {
        m_headerClickCol = HitTestHeaderColumn(pt.x);
        float currX = m_bounds.x - m_scrollX;
        for (size_t c = 0; c < m_columns.size(); ++c) {
            if (!m_columns[c].visible) continue;
            float splitterX = currX + m_columns[c].width;
            if (std::abs(pt.x - splitterX) <= 5.0f && m_columns[c].isResizable) {
                m_isResizingColumn = true;
                m_resizingColumnIndex = static_cast<int>(c);
                m_dragStartX = pt.x;
                m_initialColumnWidth = m_columns[c].width;
                m_headerClickCol = -1;
                return;
            }
            if (pt.x >= currX && pt.x < splitterX) {
                m_reorderingColumnIndex = static_cast<int>(c);
                m_columnDragStartX = pt.x;
                m_columnDragCurrentX = pt.x;
            }
            currX += GetColumnWidth(c);
        }
        return;
    }

    // 3. ReactOS LISTVIEW_LButtonDown Logic
    int clickedRow = GetRowIndexFromY(pt.y);
    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    if (clickedRow >= 0 && clickedRow < static_cast<int>(GetRowCount())) {
        m_pendingRowClick = clickedRow;
        if (m_selectionMode == ListViewSelectionMode::Single || (!ctrlDown && !shiftDown)) {
            m_selectedIndices.clear();
            m_selectedIndices.insert(clickedRow);
            m_anchorIndex = clickedRow;
            m_caretIndex = clickedRow;
        } else if (ctrlDown) {
            if (IsRowSelected(clickedRow)) {
                m_selectedIndices.erase(clickedRow);
            } else {
                m_selectedIndices.insert(clickedRow);
            }
            m_anchorIndex = clickedRow;
            m_caretIndex = clickedRow;
        } else if (shiftDown) {
            int anchor = (m_anchorIndex >= 0) ? m_anchorIndex : clickedRow;
            SelectRange(anchor, clickedRow, ctrlDown);
        }
        m_onSelectionChangedEvent.Invoke(this, clickedRow);
        StartSelectRipple(clickedRow, pt);
        InvalidateRowsLayer();
    } else {
        // Click on Empty Space / Whitespace -> Start ReactOS Marquee Select
        m_pendingRowClick = -1;
        if (!ctrlDown) {
            m_selectedIndices.clear();
        }
        m_caretIndex = -1;
        m_initialSelectedBeforeDrag = m_selectedIndices;

        m_isRubberBandSelecting = true;
        m_rubberBandStart.x = pt.x - m_bounds.x + m_scrollX;
        m_rubberBandStart.y = pt.y - m_bounds.y - m_headerHeight + m_scrollY;
        m_rubberBandCurrent = pt;
        m_rubberBandScrollOffsetY = m_scrollY;
        m_lastAutoScrollTime = std::chrono::steady_clock::now();
        InvalidateRowsLayer();
    }
}

void ListView::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);

    const bool overBar = IsOverScrollbar(m_bounds, m_headerHeight, pt, m_maxScrollY > 0.0f);
    m_scrollbarAutoHide.SetPointerOver(overBar, this);
    if (overBar) {
        RequestAnimationTicks();
    }

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
            m_scrollbarAutoHide.NotifyActivity(this);
            MarkRenderRectDirty(m_bounds);
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
            InvalidateRowsLayer();
        }
        return;
    }

    // 3. Column Reordering Drag
    if (m_isMouseDown && m_reorderingColumnIndex >= 0 && !m_isResizingColumn && !m_isDraggingScrollbar) {
        float dx = std::abs(pt.x - m_columnDragStartX);
        if (dx > 4.0f || m_isReorderingColumn) {
            m_isReorderingColumn = true;
            m_columnDragCurrentX = pt.x;
            InvalidateRowsLayer();
            return;
        }
    }

    // 4. Start Rubber-Band Drag Selection when dragging mouse
    if (m_isMouseDown && !m_isResizingColumn && !m_isReorderingColumn && !m_isDraggingScrollbar && !m_isRubberBandSelecting) {
        float dx = pt.x - m_mouseDownPoint.x;
        float dy = pt.y - m_mouseDownPoint.y;
        if (std::sqrt(dx * dx + dy * dy) > 4.0f) {
            m_isRubberBandSelecting = true;
            m_rubberBandStart.x = m_mouseDownPoint.x - m_bounds.x + m_scrollX;
            m_rubberBandStart.y = m_mouseDownPoint.y - m_bounds.y - m_headerHeight + m_scrollY;
            m_rubberBandScrollOffsetY = m_scrollY;
            m_lastAutoScrollTime = std::chrono::steady_clock::now();
        }
    }

    // 5. Rubber-Band Drag Selection Update (ReactOS LISTVIEW_MouseMove)
    if (m_isRubberBandSelecting) {
        m_rubberBandCurrent = pt;
        m_autoScrollLastMouseX = pt.x;
        m_autoScrollLastMouseY = pt.y;
        bool scrolled = ApplyAutoScroll();
        if (scrolled) {
            ClampScroll();
        }
        UpdateRubberBandSelection();
        InvalidateRowsLayer();
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

    const int newHover = GetRowIndexFromY(pt.y);
    if (newHover != m_hoveredRowIndex) {
        m_hoveredRowIndex = newHover;
        InvalidateRowsLayer();
    }
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
            InvalidateRowsLayer();
        }
    }

    if (m_headerClickCol >= 0 && !m_isReorderingColumn && !m_isResizingColumn) {
        const int col = m_headerClickCol;
        if (col >= 0 && col < static_cast<int>(m_columns.size())
            && m_columns[static_cast<size_t>(col)].sortable
            && m_columns[static_cast<size_t>(col)].visible) {
            if (m_sortColumn == col) {
                m_sortAscending = !m_sortAscending;
            } else {
                m_sortColumn = col;
                m_sortAscending = true;
            }
            m_onColumnSortEvent.Invoke(this, col, m_sortAscending);
            InvalidateRowsLayer();
            MarkRenderContentDirty();
        }
    }
    m_headerClickCol = -1;

    m_isMouseDown = false;
    m_isResizingColumn = false;
    m_isReorderingColumn = false;
    m_reorderingColumnIndex = -1;
    m_isRubberBandSelecting = false;
    m_isDraggingScrollbar = false;
    m_scrollbarAutoHide.SetDragging(false, this);
    m_pendingRowClick = -1;
    RequestAnimationTicks();
    InvalidateRowsLayer();
}

void ListView::OnMouseLeave() {
    Control::OnMouseLeave();
    m_scrollbarAutoHide.SetPointerOver(false, this);
    if (m_hoveredRowIndex != -1) {
        m_hoveredRowIndex = -1;
        InvalidateRowsLayer();
    }
    RequestAnimationTicks();
}

void ListView::OnMouseRightClick(Point pt) {
    Control::OnMouseRightClick(pt);
    m_headerContextMenuPending = false;

    if (pt.y >= m_bounds.y && pt.y <= m_bounds.y + m_headerHeight) {
        RebuildHeaderContextMenu();
        m_headerContextMenuPending = true;
        return;
    }

    int row = GetRowIndexFromY(pt.y);
    if (row < 0 || row >= static_cast<int>(GetRowCount())) return;

    // Keep rubber-band / multi-selection when right-clicking an already selected row.
    if (!IsRowSelected(row)) {
        m_selectedIndices.clear();
        m_selectedIndices.insert(row);
        m_anchorIndex = row;
        m_onSelectionChangedEvent.Invoke(this, row);
        InvalidateRowsLayer();
    }
    m_caretIndex = row;
}

void ListView::OnMouseWheel(float delta) {
    bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    if (shiftDown) {
        m_scrollX -= delta * 40.0f;
        ClampScroll();
        MarkRenderRectDirty(m_bounds);
        return;
    }

    if (m_maxScrollY <= 0.0f) {
        // Nothing to scroll — let parent ScrollViewer handle the wheel.
        UIElement::OnMouseWheel(delta);
        return;
    }

    const float prevTarget = m_targetScrollY;
    float scrollStep = m_rowHeight * 2.5f;
    m_targetScrollY -= delta * scrollStep;
    ClampScroll();
    if (std::abs(m_targetScrollY - prevTarget) < 0.001f) {
        // At scroll edge — bubble so outer page can still scroll.
        UIElement::OnMouseWheel(delta);
        return;
    }

    m_scrollYAnim.SetTarget(m_targetScrollY);
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
    } else {
        // Wheel only sets the anim target; without this, ticks stop once
        // hover/focus visual-state animation ends → intermittent frozen scroll.
        RequestAnimationTicks();
    }
    m_scrollbarAutoHide.NotifyActivity(this);
    MarkRenderRectDirty(m_bounds);
}

bool ListView::ApplyAutoScroll() {
    using clock = std::chrono::steady_clock;
    auto now = clock::now();
    if (m_lastAutoScrollTime.time_since_epoch().count() == 0) {
        m_lastAutoScrollTime = now;
        return false;
    }
    float dt = std::chrono::duration<float>(now - m_lastAutoScrollTime).count();
    dt = std::clamp(dt, 0.0001f, 0.05f);
    m_lastAutoScrollTime = now;

    float mouseX = m_autoScrollLastMouseX;
    float mouseY = m_autoScrollLastMouseY;
    bool scrolled = false;

    float contentTop = m_bounds.y + m_headerHeight;
    float contentBottom = m_bounds.y + m_bounds.height;

    if (mouseY < contentTop) {
        float dist = contentTop - mouseY;
        float speedPerSec = (std::min)(3500.0f, 250.0f + std::pow(dist / 3.0f, 1.6f) * 80.0f);
        float deltaY = speedPerSec * dt;
        m_targetScrollY = (std::max)(0.0f, m_targetScrollY - deltaY);
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
        scrolled = true;
    } else if (mouseY > contentBottom) {
        float dist = mouseY - contentBottom;
        float speedPerSec = (std::min)(3500.0f, 250.0f + std::pow(dist / 3.0f, 1.6f) * 80.0f);
        float deltaY = speedPerSec * dt;
        m_targetScrollY = (std::min)(m_maxScrollY, m_targetScrollY + deltaY);
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
        scrolled = true;
    }

    if (mouseX < m_bounds.x) {
        float dist = m_bounds.x - mouseX;
        float speedPerSec = (std::min)(3500.0f, 250.0f + std::pow(dist / 3.0f, 1.6f) * 80.0f);
        float deltaX = speedPerSec * dt;
        m_scrollX = (std::max)(0.0f, m_scrollX - deltaX);
        scrolled = true;
    } else if (mouseX > m_bounds.x + m_bounds.width) {
        float dist = mouseX - (m_bounds.x + m_bounds.width);
        float speedPerSec = (std::min)(3500.0f, 250.0f + std::pow(dist / 3.0f, 1.6f) * 80.0f);
        float deltaX = speedPerSec * dt;
        m_scrollX = (std::min)(m_maxScrollX, m_scrollX + deltaX);
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
    bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    if (ctrlDown && vkCode == 'A') {
        SelectAll();
        return;
    }

    int rowCount = static_cast<int>(GetRowCount());
    if (rowCount == 0) return;

    int newCaret = (m_caretIndex >= 0) ? m_caretIndex : 0;
    int visibleCount = static_cast<int>((m_bounds.height - m_headerHeight - 4.0f) / m_rowHeight);

    switch (vkCode) {
    case VK_UP:
        newCaret = std::max(0, newCaret - 1);
        break;
    case VK_DOWN:
        newCaret = std::min(rowCount - 1, newCaret + 1);
        break;
    case VK_PRIOR: // Page Up
        newCaret = std::max(0, newCaret - visibleCount);
        break;
    case VK_NEXT: // Page Down
        newCaret = std::min(rowCount - 1, newCaret + visibleCount);
        break;
    case VK_HOME:
        newCaret = 0;
        break;
    case VK_END:
        newCaret = rowCount - 1;
        break;
    case VK_SPACE:
        if (m_caretIndex >= 0) {
            if (m_selectionMode == ListViewSelectionMode::Extended || m_selectionMode == ListViewSelectionMode::Multiple) {
                SetRowSelected(m_caretIndex, !IsRowSelected(m_caretIndex));
            } else {
                SetRowSelected(m_caretIndex, true);
            }
        }
        return;
    }

    if (newCaret != m_caretIndex) {
        m_caretIndex = newCaret;
        EnsureVisible(m_caretIndex);

        if (m_selectionMode == ListViewSelectionMode::Extended) {
            if (shiftDown) {
                int anchor = (m_anchorIndex >= 0) ? m_anchorIndex : 0;
                SelectRange(anchor, m_caretIndex, ctrlDown);
            } else if (!ctrlDown) {
                m_selectedIndices.clear();
                m_selectedIndices.insert(m_caretIndex);
                m_anchorIndex = m_caretIndex;
                m_onSelectionChangedEvent.Invoke(this, m_caretIndex);
            }
        } else if (m_selectionMode == ListViewSelectionMode::Single) {
            m_selectedIndices.clear();
            m_selectedIndices.insert(m_caretIndex);
            m_anchorIndex = m_caretIndex;
            m_onSelectionChangedEvent.Invoke(this, m_caretIndex);
        }
        InvalidateRowsLayer();
        Rect pill = GetRowPillRect(m_caretIndex, m_scrollY);
        StartSelectRipple(m_caretIndex, Point(pill.x + pill.width * 0.5f, pill.y + pill.height * 0.5f));
    }
}

bool ListView::OnAnimationTick() {
    bool base = UIElement::OnAnimationTick();
    float dt = UIElement::GetAnimationDeltaSeconds();
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
        m_selectRippleActive = false;
        m_selectRippleCovered = true;
        const bool hideAnimating = m_scrollbarAutoHide.Tick(dt);
        if (hideAnimating) {
            MarkRenderRectDirty(m_bounds);
        }
        return base || hideAnimating;
    }

    bool rippleAnim = false;
    if (m_selectRippleActive && m_selectRippleRow >= 0) {
        Rect pill = GetRowPillRect(m_selectRippleRow, m_scrollY);
        const float cx = pill.x + m_selectRippleLocalX;
        const float cy = pill.y + m_selectRippleLocalY;
        const float corners[4][2] = {
            { pill.x, pill.y },
            { pill.x + pill.width, pill.y },
            { pill.x, pill.y + pill.height },
            { pill.x + pill.width, pill.y + pill.height },
        };
        float maxRadius = 0.0f;
        for (const auto& c : corners) {
            const float dx = cx - c[0];
            const float dy = cy - c[1];
            maxRadius = (std::max)(maxRadius, std::sqrt(dx * dx + dy * dy));
        }
        m_selectRippleRadius += (maxRadius - m_selectRippleRadius) * FrameBlend(0.146f)
            + 74.0f * dt;
        if (m_selectRippleRadius >= maxRadius - 0.5f) {
            m_selectRippleRadius = maxRadius;
            m_selectRippleCovered = true;
            m_selectRippleActive = false;
        }
        InvalidateRowsLayer();
        MarkRenderRectDirty(m_bounds);
        rippleAnim = true;
    }

    m_scrollYAnim.SetTarget(m_targetScrollY);
    const float prevScroll = m_scrollY;
    bool anim = m_scrollYAnim.Tick(dt, AnimationSpec{ 0.55f, 0.5f });
    if (anim) {
        m_scrollY = m_scrollYAnim.Current();
        m_scrollbarAutoHide.NotifyActivity(this);
        if (std::abs(m_scrollY - prevScroll) > 0.25f) {
            MarkRenderRectDirty(m_bounds);
        }
    }
    const float prevOpacity = m_scrollbarAutoHide.Opacity();
    const bool hideAnimating = m_scrollbarAutoHide.Tick(dt);
    if (std::abs(prevOpacity - m_scrollbarAutoHide.Opacity()) > 0.001f) {
        MarkRenderRectDirty(m_bounds);
    }
    if (anim || hideAnimating || rippleAnim) {
        RequestAnimationTicks();
    }
    return base || anim || hideAnimating || rippleAnim;
}

bool ListView::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || std::abs(m_scrollYAnim.Target() - m_scrollYAnim.Current()) > 0.001f
        || m_scrollbarAutoHide.NeedsTicks()
        || m_selectRippleActive;
}

Rect ListView::GetRowPillRect(int row, float scrollY) const {
    const float rowY = m_bounds.y + m_headerHeight + row * m_rowHeight - scrollY;
    const float totalColsW = GetTotalColumnsWidth();
    const float rowW = std::max(m_bounds.width, totalColsW);
    const float pillW = (std::max)(0.0f, (std::min)(rowW, m_bounds.width) - kPillInsetX * 2.0f);
    const float pillH = (std::max)(0.0f, m_rowHeight - kPillInsetY * 2.0f);
    return Rect(m_bounds.x + kPillInsetX, rowY + kPillInsetY, pillW, pillH);
}

void ListView::StartSelectRipple(int row, Point pt) {
    if (row < 0 || row >= static_cast<int>(GetRowCount())) return;
    if (!UIElement::AreAnimationsEnabled()) {
        m_selectRippleActive = false;
        m_selectRippleCovered = true;
        m_selectRippleRow = row;
        return;
    }

    Rect pill = GetRowPillRect(row, m_scrollY);
    m_selectRippleRow = row;
    m_selectRippleLocalX = std::clamp(pt.x - pill.x, 0.0f, (std::max)(0.0f, pill.width));
    m_selectRippleLocalY = std::clamp(pt.y - pill.y, 0.0f, (std::max)(0.0f, pill.height));
    // Prefer center if click is somehow outside the pill.
    if (pill.width > 0.0f && (pt.x < pill.x || pt.x > pill.x + pill.width)) {
        m_selectRippleLocalX = pill.width * 0.5f;
    }
    if (pill.height > 0.0f && (pt.y < pill.y || pt.y > pill.y + pill.height)) {
        m_selectRippleLocalY = pill.height * 0.5f;
    }
    m_selectRippleRadius = 4.0f;
    m_selectRippleActive = true;
    m_selectRippleCovered = false;
    RequestAnimationTicks();
}

} // namespace CUI
