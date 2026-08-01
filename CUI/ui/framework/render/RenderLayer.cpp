#include "RenderLayer.h"

namespace CUI {

void RenderLayer::Invalidate(unsigned flags) {
    m_dirtyFlags |= flags;
    m_valid = false;
}

void RenderLayer::Validate() {
    m_dirtyFlags = None;
    m_valid = true;
    m_lastRenderedBounds = m_bounds;
}

} // namespace CUI
