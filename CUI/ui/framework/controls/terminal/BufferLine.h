#pragma once
#include "CellData.h"
#include <string>
#include <vector>

namespace CUI {
namespace Term {

class BufferLine {
public:
    explicit BufferLine(int cols);

    int Length() const { return static_cast<int>(m_cells.size()); }

    CellData& operator[](int index) { return m_cells[static_cast<size_t>(index)]; }
    const CellData& operator[](int index) const { return m_cells[static_cast<size_t>(index)]; }

    bool IsWrapped() const { return m_isWrapped; }
    void SetIsWrapped(bool wrapped) { m_isWrapped = wrapped; }

    bool IsDirty() const { return m_isDirty; }
    void SetIsDirty(bool dirty) { m_isDirty = dirty; }
    void MarkDirty() { m_isDirty = true; }

    void Resize(int cols);
    void Fill(const CellData& cell, int start = 0, int end = -1);
    void Clear();

    std::wstring GetTrimmedText() const;
    std::wstring GetText(int start, int end) const;

private:
    std::vector<CellData> m_cells;
    bool m_isWrapped = false;
    bool m_isDirty = true;
};

} // namespace Term
} // namespace CUI
