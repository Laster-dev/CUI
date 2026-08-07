#pragma once

#include "CompositionLayer.h"
#include <functional>
#include <vector>

namespace CUI {

class GraphicsContext;

// Rasterizes dirty PictureLayers. Currently runs synchronously on the UI thread;
// the API is ready for a worker with ID3D11Multithread when enabled.
class LayerRasterizer {
public:
    using RasterJob = std::function<void(GraphicsContext&)>;

    void Enqueue(RasterJob job);
    // Drain all jobs on the calling thread (UI). Returns number executed.
    int FlushSync(GraphicsContext& ctx);

    void SetAsyncEnabled(bool enabled) { m_asyncEnabled = enabled; }
    bool IsAsyncEnabled() const { return m_asyncEnabled; }

private:
    std::vector<RasterJob> m_jobs;
    bool m_asyncEnabled = false; // async path not enabled yet — sync fallback is correct.
};

} // namespace CUI
