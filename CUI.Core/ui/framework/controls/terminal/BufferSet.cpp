#include "BufferSet.h"

namespace CUI {
namespace Term {

BufferSet::BufferSet(int cols, int rows, int scrollback)
    : m_normal(cols, rows, scrollback)
    , m_alt(cols, rows, 0)
    , m_active(&m_normal) {
}

void BufferSet::ActivateNormal() {
    m_active = &m_normal;
}

void BufferSet::ActivateAlt(bool clear) {
    m_active = &m_alt;
    if (clear) {
        m_alt.ClearViewport();
    }
}

void BufferSet::Resize(int cols, int rows) {
    m_normal.Resize(cols, rows);
    m_alt.Resize(cols, rows);
}

void BufferSet::Reset() {
    m_normal.Reset();
    m_alt.Reset();
    m_active = &m_normal;
}

} // namespace Term
} // namespace CUI
