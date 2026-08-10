#pragma once
#include "TerminalBuffer.h"

namespace CUI {
namespace Term {

class BufferSet {
public:
    BufferSet(int cols, int rows, int scrollback);

    TerminalBuffer& Normal() { return m_normal; }
    TerminalBuffer& Alt() { return m_alt; }
    TerminalBuffer& Active() { return *m_active; }
    const TerminalBuffer& Active() const { return *m_active; }

    bool IsAltActive() const { return m_active == &m_alt; }

    void ActivateNormal();
    void ActivateAlt(bool clear = true);

    void Resize(int cols, int rows);
    void Reset();

private:
    TerminalBuffer m_normal;
    TerminalBuffer m_alt;
    TerminalBuffer* m_active;
};

} // namespace Term
} // namespace CUI
