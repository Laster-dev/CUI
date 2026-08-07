#include "LayerRasterizer.h"
#include "GraphicsContext.h"

namespace CUI {

void LayerRasterizer::Enqueue(RasterJob job) {
    if (job) {
        m_jobs.push_back(std::move(job));
    }
}

int LayerRasterizer::FlushSync(GraphicsContext& ctx) {
    int count = 0;
    auto jobs = std::move(m_jobs);
    m_jobs.clear();
    for (auto& job : jobs) {
        if (job) {
            job(ctx);
            ++count;
        }
    }
    (void)m_asyncEnabled;
    return count;
}

} // namespace CUI
