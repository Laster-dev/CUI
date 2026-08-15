#include "RenderLayer.h"

namespace CUI {

void RenderLayer::Invalidate(unsigned flags) {
    m_dirtyFlags |= flags;
    // Size/structure/clip require discarding the bitmap. ContentDirty alone must
    // KEEP m_valid so ScrollViewer can strip-patch ripples/hover into the existing
    // cache — otherwise every MarkRenderRectDirty forces FULL_RERASTER (PropertyGrid
    // / NavigationView menu hitch identical to the old scroll bug).
    constexpr unsigned kDiscardBitmapMask = SizeDirty | StructureDirty | ClipDirty;
    if (flags & kDiscardBitmapMask) {
        m_valid = false;
    }
}

void RenderLayer::ClearDirtyFlags(unsigned flags) {
    m_dirtyFlags &= ~flags;
}

bool RenderLayer::NeedsContentRaster() const {
    constexpr unsigned kContentMask =
        ContentDirty | SizeDirty | StructureDirty | ClipDirty;
    return !m_cacheBitmap || !m_valid || (m_dirtyFlags & kContentMask) != 0;
}

bool RenderLayer::NeedsComposeOnly() const {
    constexpr unsigned kContentMask =
        ContentDirty | SizeDirty | StructureDirty | ClipDirty;
    constexpr unsigned kComposeMask = OpacityDirty | TransformDirty;
    return m_cacheBitmap
        && m_valid
        && (m_dirtyFlags & kContentMask) == 0
        && (m_dirtyFlags & kComposeMask) != 0;
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
    m_stamp = RenderCacheStamp{};
    m_valid = false;
    m_dirtyFlags = ContentDirty | TransformDirty | ClipDirty | SizeDirty | StructureDirty;
}

} // namespace CUI
