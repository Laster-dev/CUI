#include "BufferLine.h"
#include "UnicodeWidth.h"
#include <algorithm>

namespace CUI {
namespace Term {

BufferLine::BufferLine(int cols) {
    if (cols < 0) cols = 0;
    m_cells.assign(static_cast<size_t>(cols), CellData::Empty());
}

void BufferLine::Resize(int cols) {
    if (cols < 0) cols = 0;
    if (static_cast<size_t>(cols) == m_cells.size()) {
        return;
    }

    std::vector<CellData> next(static_cast<size_t>(cols), CellData::Empty());
    const size_t copy = (std::min)(static_cast<size_t>(cols), m_cells.size());
    for (size_t i = 0; i < copy; ++i) {
        next[i] = m_cells[i];
    }
    m_cells.swap(next);
    m_isDirty = true;
}

void BufferLine::Fill(const CellData& cell, int start, int end) {
    if (end < 0) {
        end = static_cast<int>(m_cells.size());
    }
    if (start < 0) start = 0;
    if (end > static_cast<int>(m_cells.size())) {
        end = static_cast<int>(m_cells.size());
    }
    for (int i = start; i < end; ++i) {
        m_cells[static_cast<size_t>(i)].SetFrom(cell);
    }
    m_isDirty = true;
}

void BufferLine::Clear() {
    for (auto& cell : m_cells) {
        cell.Reset();
    }
    m_isWrapped = false;
    m_isDirty = true;
}

std::wstring BufferLine::GetTrimmedText() const {
    int end = static_cast<int>(m_cells.size());
    while (end > 0) {
        const CellData& cell = m_cells[static_cast<size_t>(end - 1)];
        if (cell.GetWidth() == 0) {
            if (end >= 2 && !m_cells[static_cast<size_t>(end - 2)].IsEmpty()) {
                break;
            }
        }
        const int ch = cell.GetCodePoint();
        if (ch != 0 && ch != ' ') {
            break;
        }
        end--;
    }

    if (end == 0) {
        return std::wstring();
    }

    return GetText(0, end);
}

std::wstring BufferLine::GetText(int start, int end) const {
    const int len = static_cast<int>(m_cells.size());
    start = std::clamp(start, 0, len);
    end = std::clamp(end, 0, len);
    if (end <= start) {
        return std::wstring();
    }

    std::wstring result;
    result.reserve(static_cast<size_t>(end - start));
    for (int i = start; i < end; ++i) {
        const CellData& cell = m_cells[static_cast<size_t>(i)];
        if (cell.GetWidth() == 0) {
            continue;
        }
        int cp = cell.GetCodePoint();
        if (cp == 0) {
            cp = ' ';
        }
        AppendUtf16CodePoint(result, cp);
    }
    return result;
}

} // namespace Term
} // namespace CUI
