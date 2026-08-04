#pragma once
#include "BufferLine.h"
#include <deque>
#include <memory>
#include <string>

namespace CUI {
namespace Term {

class TerminalBuffer {
public:
    TerminalBuffer(int cols, int rows, int scrollback);

    int Cols() const { return m_cols; }
    int Rows() const { return m_rows; }

    int CursorX = 0;
    int CursorY = 0;
    int SavedCursorX = 0;
    int SavedCursorY = 0;
    int ScrollTop = 0;
    int ScrollBottom = 0;
    bool OriginMode = false;
    bool Wraparound = true;
    bool InsertMode = false;
    CellData CurAttr = CellData::Empty();
    int ActiveLinkId = 0;

    // View offset into scrollback; 0 means bottom (live viewport).
    int YDisp = 0;

    int BaseY() const;
    int Length() const { return static_cast<int>(m_lines.size()); }

    BufferLine& GetLine(int absoluteY);
    BufferLine& GetViewportLine(int row);
    BufferLine& GetCursorLine();

    void Resize(int cols, int rows);
    void Reset();
    void ClearViewport();

    void PrintChar(int codePoint, int width = 1);
    void CarriageReturn() { CursorX = 0; }
    void LineFeed();
    void ReverseIndex();
    void Index() { LineFeed(); }

    void ScrollUp(int n);
    void ScrollDown(int n);

    void EraseInDisplay(int mode);
    void EraseInLine(int mode);

    void DeleteChars(int n);
    void InsertChars(int n);
    void EraseChars(int n);
    void DeleteLines(int n);
    void InsertLines(int n);

    void SaveCursor();
    void RestoreCursor();
    void SetCursor(int x, int y);
    void SetScrollRegion(int top, int bottom);

    // rows are absolute buffer indices
    std::wstring GetSelectedText(int startCol, int startRow, int endCol, int endRow) const;

private:
    void Reflow(int newCols);
    void InsertCells(int n);
    CellData EraseCell() const;
    void EnsureViewportLines();
    void EnsureLine(int absoluteY);
    void TrimScrollback();
    // Full-screen scroll: recycle the dropped scrollback line as the new blank row
    // so a `yes` flood does not allocate a BufferLine per line feed.
    void ScrollFullScreenBy(int n);
    std::unique_ptr<BufferLine> MakeLine() const;

    // deque: TrimScrollback / full-screen ScrollUp need O(1) pop_front.
    // vector::erase(begin) is O(n) and freezes the UI under rapid output.
    std::deque<std::unique_ptr<BufferLine>> m_lines;
    // Returned for out-of-range viewport queries (mirrors GetOrCreateBlank()).
    mutable std::unique_ptr<BufferLine> m_blank;
    int m_scrollbackMax = 0;
    int m_cols = 1;
    int m_rows = 1;
};

} // namespace Term
} // namespace CUI
