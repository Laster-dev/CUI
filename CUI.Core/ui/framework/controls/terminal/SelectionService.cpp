#include "SelectionService.h"
#include <algorithm>
#include <cwctype>

namespace CUI {
namespace Term {

void SelectionModel::Normalize() {
    if (StartRow > EndRow || (StartRow == EndRow && StartCol > EndCol)) {
        std::swap(StartCol, EndCol);
        std::swap(StartRow, EndRow);
    }
}

void SelectionModel::GetOrdered(int& startCol, int& startRow, int& endCol, int& endRow) const {
    startCol = StartCol;
    startRow = StartRow;
    endCol = EndCol;
    endRow = EndRow;
    if (startRow > endRow || (startRow == endRow && startCol > endCol)) {
        std::swap(startCol, endCol);
        std::swap(startRow, endRow);
    }
}

bool SelectionModel::Contains(int absRow, int col) const {
    if (!HasSelection) {
        return false;
    }
    int startCol = 0, startRow = 0, endCol = 0, endRow = 0;
    GetOrdered(startCol, startRow, endCol, endRow);
    if (absRow < startRow || absRow > endRow) {
        return false;
    }
    if (startRow == endRow) {
        return col >= startCol && col <= endCol;
    }
    if (absRow == startRow) {
        return col >= startCol;
    }
    if (absRow == endRow) {
        return col <= endCol;
    }
    return true;
}

void SelectionService::Begin(int absRow, int col) {
    m_isSelecting = true;
    m_model.HasSelection = true;
    m_model.StartCol = m_model.EndCol = col;
    m_model.StartRow = m_model.EndRow = absRow;
}

void SelectionService::Update(int absRow, int col) {
    if (!m_isSelecting) {
        return;
    }
    m_model.EndCol = col;
    m_model.EndRow = absRow;
    m_model.HasSelection = true;
}

void SelectionService::End() {
    m_isSelecting = false;
    m_model.Normalize();
    if (m_model.StartRow == m_model.EndRow && m_model.StartCol == m_model.EndCol) {
        m_model.Clear();
    }
}

void SelectionService::SelectWord(int absRow, int col, const std::function<BufferLine*(int)>& getLine, int cols) {
    if (!getLine) {
        return;
    }
    BufferLine* line = getLine(absRow);
    if (line == nullptr) {
        return;
    }

    col = std::clamp(col, 0, (std::max)(0, cols - 1));
    int start = col;
    int end = col;
    while (start > 0 && start - 1 < line->Length() && IsWordChar((*line)[start - 1].GetChar())) {
        start--;
    }
    while (end < cols - 1 && end + 1 < line->Length() && IsWordChar((*line)[end + 1].GetChar())) {
        end++;
    }

    m_model.HasSelection = true;
    m_model.StartRow = m_model.EndRow = absRow;
    m_model.StartCol = start;
    m_model.EndCol = end;
    m_isSelecting = false;
}

void SelectionService::SelectLine(int absRow, int cols) {
    m_model.HasSelection = true;
    m_model.StartRow = m_model.EndRow = absRow;
    m_model.StartCol = 0;
    m_model.EndCol = cols - 1;
    m_isSelecting = false;
}

void SelectionService::Clear() {
    m_isSelecting = false;
    m_model.Clear();
}

bool SelectionService::IsWordChar(wchar_t c) {
    return std::iswalnum(static_cast<wint_t>(c)) != 0 || c == L'_' || c == L'-' || c == L'.';
}

} // namespace Term
} // namespace CUI
