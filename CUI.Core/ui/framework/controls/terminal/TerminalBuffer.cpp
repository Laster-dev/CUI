#include "TerminalBuffer.h"
#include <algorithm>

namespace CUI {
namespace Term {

namespace {
std::wstring TrimEnd(const std::wstring& s) {
    size_t end = s.size();
    while (end > 0) {
        const wchar_t c = s[end - 1];
        if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n' && c != L'\v' && c != L'\f' && c != L'\0') {
            break;
        }
        end--;
    }
    return s.substr(0, end);
}
}

TerminalBuffer::TerminalBuffer(int cols, int rows, int scrollback) {
    m_cols = (std::max)(1, cols);
    m_rows = (std::max)(1, rows);
    m_scrollbackMax = (std::max)(0, scrollback);
    CursorX = 0;
    CursorY = 0;
    SavedCursorX = 0;
    SavedCursorY = 0;
    EnsureViewportLines();
}

std::unique_ptr<BufferLine> TerminalBuffer::MakeLine() const {
    return std::make_unique<BufferLine>(m_cols);
}

int TerminalBuffer::BaseY() const {
    return (std::max)(0, static_cast<int>(m_lines.size()) - m_rows);
}

BufferLine& TerminalBuffer::GetLine(int absoluteY) {
    if (absoluteY < 0) absoluteY = 0;
    if (absoluteY >= static_cast<int>(m_lines.size())) {
        EnsureLine(absoluteY);
    }
    return *m_lines[static_cast<size_t>(absoluteY)];
}

BufferLine& TerminalBuffer::GetViewportLine(int row) {
    const int y = BaseY() - YDisp + row;
    if (y < 0 || y >= static_cast<int>(m_lines.size())) {
        if (!m_blank || m_blank->Length() != m_cols) {
            m_blank = std::make_unique<BufferLine>(m_cols);
        } else {
            m_blank->Clear();
        }
        return *m_blank;
    }
    return *m_lines[static_cast<size_t>(y)];
}

BufferLine& TerminalBuffer::GetCursorLine() {
    const int y = BaseY() + CursorY;
    EnsureLine(y);
    return *m_lines[static_cast<size_t>(y)];
}

void TerminalBuffer::Resize(int cols, int rows) {
    cols = (std::max)(1, cols);
    rows = (std::max)(1, rows);

    if (cols != m_cols) {
        Reflow(cols);
    }

    m_cols = cols;
    m_rows = rows;
    ScrollTop = 0;
    ScrollBottom = m_rows - 1;

    // Reflow rebuilt the lines at the new column count; anything that survived a
    // pure row change still carries the old width.
    for (auto& line : m_lines) {
        if (line->Length() != m_cols) {
            line->Resize(m_cols);
        }
    }

    EnsureViewportLines();
    CursorX = (std::min)(CursorX, m_cols - 1);
    CursorY = (std::min)(CursorY, m_rows - 1);
    if (CursorX < 0) CursorX = 0;
    if (CursorY < 0) CursorY = 0;
    YDisp = 0;
}

void TerminalBuffer::Reflow(int newCols) {
    if (m_lines.empty()) {
        m_cols = newCols;
        return;
    }

    std::vector<std::vector<CellData>> segments;
    std::vector<CellData> current;
    bool hasCurrent = false;

    for (size_t i = 0; i < m_lines.size(); ++i) {
        BufferLine& line = *m_lines[i];
        if (!hasCurrent) {
            current.clear();
            hasCurrent = true;
        }
        for (int c = 0; c < line.Length(); ++c) {
            CellData& cell = line[c];
            if (cell.GetWidth() == 0) {
                continue; // skip wide-char trailers
            }
            current.push_back(cell);
        }

        if (!line.IsWrapped()) {
            // Trim trailing spaces for hard break
            while (!current.empty() && current.back().IsEmpty()) {
                current.pop_back();
            }
            segments.push_back(current);
            current.clear();
            hasCurrent = false;
        }
    }
    if (hasCurrent && !current.empty()) {
        segments.push_back(current);
    }

    m_lines.clear();
    m_cols = newCols;

    for (const auto& seg : segments) {
        if (seg.empty()) {
            m_lines.push_back(MakeLine());
            continue;
        }

        int col = 0;
        BufferLine* row = nullptr;
        for (size_t i = 0; i < seg.size(); ++i) {
            const CellData& cell = seg[i];
            const int w = (std::max)(1, cell.GetWidth());
            if (row == nullptr || col + w > newCols) {
                if (row != nullptr) {
                    row->SetIsWrapped(true);
                }
                m_lines.push_back(MakeLine());
                row = m_lines.back().get();
                col = 0;
            }

            (*row)[col].SetFrom(cell);
            for (int t = 1; t < w && col + t < newCols; ++t) {
                CellData trail = cell;
                trail.SetCodePoint(0);
                trail.SetWidth(0);
                (*row)[col + t].SetFrom(trail);
            }
            col += w;
        }
    }

    TrimScrollback();
}

void TerminalBuffer::Reset() {
    m_lines.clear();
    CursorX = 0;
    CursorY = 0;
    SavedCursorX = 0;
    SavedCursorY = 0;
    ScrollTop = 0;
    ScrollBottom = m_rows - 1;
    OriginMode = false;
    Wraparound = true;
    InsertMode = false;
    CurAttr = CellData::Empty();
    ActiveLinkId = 0;
    YDisp = 0;
    EnsureViewportLines();
}

void TerminalBuffer::ClearViewport() {
    const int base = BaseY();
    for (int i = 0; i < m_rows; ++i) {
        const int y = base + i;
        EnsureLine(y);
        m_lines[static_cast<size_t>(y)]->Clear();
    }
    CursorX = 0;
    CursorY = 0;
}

void TerminalBuffer::PrintChar(int codePoint, int width) {
    if (width <= 0) {
        width = 1;
    }

    if (CursorX >= m_cols) {
        if (!Wraparound) {
            CursorX = m_cols - 1;
        } else {
            GetCursorLine().SetIsWrapped(true);
            CursorX = 0;
            LineFeed();
        }
    }

    BufferLine& line = GetCursorLine();
    if (InsertMode && CursorX < m_cols) {
        InsertCells(1);
    }

    CellData cell = CurAttr;
    cell.SetCodePoint(codePoint);
    cell.SetWidth(width);
    cell.LinkId = ActiveLinkId;
    line[CursorX].SetFrom(cell);

    // Clear following cells for wide chars
    for (int i = 1; i < width && CursorX + i < m_cols; ++i) {
        CellData empty = CurAttr;
        empty.SetCodePoint(0);
        empty.SetWidth(0);
        line[CursorX + i].SetFrom(empty);
    }

    line.MarkDirty();
    CursorX += width;
}

void TerminalBuffer::LineFeed() {
    if (CursorY == ScrollBottom) {
        ScrollUp(1);
    } else if (CursorY < m_rows - 1) {
        CursorY++;
    }
}

void TerminalBuffer::ReverseIndex() {
    if (CursorY == ScrollTop) {
        ScrollDown(1);
    } else if (CursorY > 0) {
        CursorY--;
    }
}

void TerminalBuffer::ScrollFullScreenBy(int n) {
    n = (std::max)(1, n);
    const size_t max = static_cast<size_t>(m_rows) + static_cast<size_t>(m_scrollbackMax);
    for (int i = 0; i < n; ++i) {
        if (m_lines.size() >= max && !m_lines.empty()) {
            std::unique_ptr<BufferLine> recycled = std::move(m_lines.front());
            m_lines.pop_front();
            recycled->Fill(EraseCell());
            recycled->SetIsWrapped(false);
            recycled->MarkDirty();
            m_lines.push_back(std::move(recycled));
        } else {
            m_lines.push_back(MakeLine());
        }
    }
}

void TerminalBuffer::ScrollUp(int n) {
    n = (std::max)(1, n);
    if (ScrollTop == 0 && ScrollBottom == m_rows - 1) {
        ScrollFullScreenBy(n);
        return;
    }

    const int top = BaseY() + ScrollTop;
    const int bottom = BaseY() + ScrollBottom;
    for (int i = 0; i < n; ++i) {
        if (top < 0 || top >= static_cast<int>(m_lines.size())) {
            continue;
        }
        m_lines.erase(m_lines.begin() + top);
        const int insertAt = (std::min)(bottom, static_cast<int>(m_lines.size()));
        m_lines.insert(m_lines.begin() + insertAt, MakeLine());
    }
}

void TerminalBuffer::ScrollDown(int n) {
    n = (std::max)(1, n);
    const int top = BaseY() + ScrollTop;
    const int bottom = BaseY() + ScrollBottom;
    for (int i = 0; i < n; ++i) {
        if (bottom < 0 || bottom >= static_cast<int>(m_lines.size())) {
            continue;
        }
        m_lines.erase(m_lines.begin() + bottom);
        const int insertAt = (std::min)(top, static_cast<int>(m_lines.size()));
        m_lines.insert(m_lines.begin() + insertAt, MakeLine());
    }
}

void TerminalBuffer::EraseInDisplay(int mode) {
    const CellData blank = EraseCell();
    switch (mode) {
    case 0: // cursor to end
        EraseInLine(0);
        for (int y = CursorY + 1; y < m_rows; ++y) {
            EnsureLine(BaseY() + y);
            m_lines[static_cast<size_t>(BaseY() + y)]->Fill(blank);
        }
        break;
    case 1: // start to cursor
        for (int y = 0; y < CursorY; ++y) {
            EnsureLine(BaseY() + y);
            m_lines[static_cast<size_t>(BaseY() + y)]->Fill(blank);
        }
        EraseInLine(1);
        break;
    case 2: // entire viewport
    case 3: // viewport + scrollback
        if (mode == 3) {
            m_lines.clear();
            EnsureViewportLines();
        } else {
            for (int y = 0; y < m_rows; ++y) {
                EnsureLine(BaseY() + y);
                m_lines[static_cast<size_t>(BaseY() + y)]->Fill(blank);
            }
        }
        break;
    default:
        break;
    }
}

void TerminalBuffer::EraseInLine(int mode) {
    BufferLine& line = GetCursorLine();
    const CellData blank = EraseCell();
    switch (mode) {
    case 0:
        line.Fill(blank, CursorX, m_cols);
        break;
    case 1:
        line.Fill(blank, 0, CursorX + 1);
        break;
    case 2:
        line.Fill(blank);
        break;
    default:
        break;
    }
}

void TerminalBuffer::DeleteChars(int n) {
    n = (std::max)(1, n);
    BufferLine& line = GetCursorLine();
    const CellData blank = EraseCell();
    for (int i = CursorX; i < m_cols; ++i) {
        const int src = i + n;
        if (src < m_cols) {
            line[i].SetFrom(line[src]);
        } else {
            line[i].SetFrom(blank);
        }
    }
    line.MarkDirty();
}

void TerminalBuffer::InsertChars(int n) {
    n = (std::max)(1, n);
    InsertCells(n);
}

void TerminalBuffer::EraseChars(int n) {
    n = (std::max)(1, n);
    BufferLine& line = GetCursorLine();
    const CellData blank = EraseCell();
    const int end = (std::min)(m_cols, CursorX + n);
    line.Fill(blank, CursorX, end);
}

void TerminalBuffer::DeleteLines(int n) {
    n = (std::max)(1, n);
    const int top = BaseY() + CursorY;
    const int bottom = BaseY() + ScrollBottom;
    for (int i = 0; i < n; ++i) {
        if (top <= bottom && top >= 0 && top < static_cast<int>(m_lines.size())) {
            m_lines.erase(m_lines.begin() + top);
            const int insertAt = (std::min)(bottom, static_cast<int>(m_lines.size()));
            m_lines.insert(m_lines.begin() + insertAt, MakeLine());
        }
    }
}

void TerminalBuffer::InsertLines(int n) {
    n = (std::max)(1, n);
    const int top = BaseY() + CursorY;
    const int bottom = BaseY() + ScrollBottom;
    for (int i = 0; i < n; ++i) {
        if (top <= bottom && bottom >= 0 && bottom < static_cast<int>(m_lines.size())) {
            m_lines.erase(m_lines.begin() + bottom);
            const int insertAt = (std::min)(top, static_cast<int>(m_lines.size()));
            m_lines.insert(m_lines.begin() + insertAt, MakeLine());
        }
    }
}

void TerminalBuffer::SaveCursor() {
    SavedCursorX = CursorX;
    SavedCursorY = CursorY;
}

void TerminalBuffer::RestoreCursor() {
    CursorX = std::clamp(SavedCursorX, 0, m_cols - 1);
    CursorY = std::clamp(SavedCursorY, 0, m_rows - 1);
}

void TerminalBuffer::SetCursor(int x, int y) {
    CursorX = std::clamp(x, 0, m_cols - 1);
    const int minY = OriginMode ? ScrollTop : 0;
    const int maxY = OriginMode ? ScrollBottom : m_rows - 1;
    CursorY = std::clamp(y, minY, (std::max)(minY, maxY));
}

void TerminalBuffer::SetScrollRegion(int top, int bottom) {
    top = std::clamp(top, 0, m_rows - 1);
    bottom = std::clamp(bottom, 0, m_rows - 1);
    if (bottom < top) {
        bottom = top;
    }
    ScrollTop = top;
    ScrollBottom = bottom;
    CursorX = 0;
    CursorY = OriginMode ? ScrollTop : 0;
}

std::wstring TerminalBuffer::GetSelectedText(int startCol, int startRow, int endCol, int endRow) const {
    if (startRow > endRow || (startRow == endRow && startCol > endCol)) {
        std::swap(startCol, endCol);
        std::swap(startRow, endRow);
    }

    std::wstring result;
    for (int y = startRow; y <= endRow; ++y) {
        if (y < 0 || y >= static_cast<int>(m_lines.size())) {
            continue;
        }
        const BufferLine& line = *m_lines[static_cast<size_t>(y)];
        const int s = (y == startRow) ? startCol : 0;
        int e = m_cols;
        if (y == endRow) {
            e = endCol + 1;
            if (endCol >= 0 && endCol < line.Length() && line[endCol].GetWidth() >= 2) {
                e = endCol + line[endCol].GetWidth();
            }
        }

        std::wstring lineText = line.GetText(s, e);
        if (y == endRow || !line.IsWrapped()) {
            lineText = TrimEnd(lineText);
        }

        result += lineText;

        if (y < endRow && !line.IsWrapped()) {
            result += L"\r\n";
        }
    }
    return result;
}

void TerminalBuffer::InsertCells(int n) {
    BufferLine& line = GetCursorLine();
    for (int i = m_cols - 1; i >= CursorX + n; --i) {
        line[i].SetFrom(line[i - n]);
    }
    const CellData blank = EraseCell();
    const int end = (std::min)(m_cols, CursorX + n);
    line.Fill(blank, CursorX, end);
}

CellData TerminalBuffer::EraseCell() const {
    CellData cell = CurAttr;
    cell.SetCodePoint(' ');
    cell.SetWidth(1);
    // Erase uses current bg typically; keep attrs minimal
    cell.Attrs = 0;
    return cell;
}

void TerminalBuffer::EnsureViewportLines() {
    while (static_cast<int>(m_lines.size()) < m_rows) {
        m_lines.push_back(MakeLine());
    }
    ScrollTop = 0;
    ScrollBottom = m_rows - 1;
}

void TerminalBuffer::EnsureLine(int absoluteY) {
    while (static_cast<int>(m_lines.size()) <= absoluteY) {
        m_lines.push_back(MakeLine());
    }
}

void TerminalBuffer::TrimScrollback() {
    const size_t max = static_cast<size_t>(m_rows) + static_cast<size_t>(m_scrollbackMax);
    while (m_lines.size() > max) {
        m_lines.pop_front();
    }
}

} // namespace Term
} // namespace CUI
