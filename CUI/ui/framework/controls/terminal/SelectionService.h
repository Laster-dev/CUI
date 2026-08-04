#pragma once
#include "BufferLine.h"
#include <functional>

namespace CUI {
namespace Term {

struct SelectionModel {
    bool HasSelection = false;
    int StartCol = 0;
    int StartRow = 0; // absolute buffer row
    int EndCol = 0;
    int EndRow = 0;

    void Clear() {
        HasSelection = false;
        StartCol = StartRow = EndCol = EndRow = 0;
    }

    // Swaps endpoints so Start <= End. Only call when selection is finished
    // (mouse up / copy); never during drag paint — that would drop the anchor.
    void Normalize();

    // Non-mutating ordered bounds for hit-test and painting while dragging.
    void GetOrdered(int& startCol, int& startRow, int& endCol, int& endRow) const;

    bool Contains(int absRow, int col) const;
};

class SelectionService {
public:
    SelectionModel& Model() { return m_model; }
    const SelectionModel& Model() const { return m_model; }

    bool IsSelecting() const { return m_isSelecting; }

    void Begin(int absRow, int col);
    void Update(int absRow, int col);
    void End();

    void SelectWord(int absRow, int col, const std::function<BufferLine*(int)>& getLine, int cols);
    void SelectLine(int absRow, int cols);
    void Clear();

private:
    static bool IsWordChar(wchar_t c);

    SelectionModel m_model;
    bool m_isSelecting = false;
};

} // namespace Term
} // namespace CUI
