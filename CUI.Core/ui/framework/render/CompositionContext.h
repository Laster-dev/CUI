#pragma once

#include "DirtyRegion.h"

namespace CUI {

class CompositionContext {
public:
    struct FrameStats {
        unsigned dirtyRectCount = 0;
        unsigned rasterizedNodeCount = 0;
        unsigned layoutPassCount = 0;
        unsigned layerCacheHitCount = 0;
        unsigned layerCacheMissCount = 0;
        unsigned layerCacheRerenderCount = 0;
        unsigned layerCacheReuseCount = 0;
        unsigned displayListOpCount = 0;
        unsigned displayListCacheHitCount = 0;
        unsigned displayListBatchReductionCount = 0;
        bool fullRepaint = false;
    };

    void BeginFrame(const Rect& viewportBounds, const DirtyRegion& dirtyRegion, bool fullRepaint);
    void EndFrame();

    const DirtyRegion& GetDirtyRegion() const { return m_dirtyRegion; }
    const Rect& GetViewportBounds() const { return m_viewportBounds; }
    bool IsFullRepaint() const { return m_fullRepaint; }

    void CountRasterizedNode() { ++m_stats.rasterizedNodeCount; }
    void CountLayoutPass() { ++m_stats.layoutPassCount; }
    void CountLayerCacheHit() { ++m_stats.layerCacheHitCount; }
    void CountLayerCacheMiss() { ++m_stats.layerCacheMissCount; }
    void CountLayerCacheRerender() { ++m_stats.layerCacheRerenderCount; }
    void CountLayerCacheReuse() { ++m_stats.layerCacheReuseCount; }
    void CountDisplayListOps(unsigned count) { m_stats.displayListOpCount += count; }
    void CountDisplayListCacheHit() { ++m_stats.displayListCacheHitCount; }
    void CountDisplayListBatchReduction(unsigned count) { m_stats.displayListBatchReductionCount += count; }
    const FrameStats& GetStats() const { return m_stats; }

private:
    DirtyRegion m_dirtyRegion;
    Rect m_viewportBounds;
    bool m_fullRepaint = true;
    FrameStats m_stats;
};

} // namespace CUI
