#include "CompositionContext.h"

namespace CUI {

void CompositionContext::BeginFrame(const Rect& viewportBounds, const DirtyRegion& dirtyRegion, bool fullRepaint) {
    m_viewportBounds = viewportBounds;
    m_dirtyRegion = dirtyRegion;
    m_fullRepaint = fullRepaint;
    m_stats = {};
    m_stats.dirtyRectCount = static_cast<unsigned>(dirtyRegion.GetRectCount());
    m_stats.fullRepaint = fullRepaint;
}

void CompositionContext::EndFrame() {
}

} // namespace CUI
