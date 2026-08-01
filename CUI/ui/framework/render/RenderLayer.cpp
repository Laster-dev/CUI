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

void RenderLayer::ResetCache() {
    m_cacheContext.Reset();
    m_cacheBitmap.Reset();
    m_scratchBitmap.Reset();
    m_cacheSurfaceSize = Size();
    m_valid = false;
}

} // namespace CUI
