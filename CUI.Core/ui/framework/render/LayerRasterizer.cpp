#include "LayerRasterizer.h"
#include "GraphicsContext.h"

namespace CUI {

LayerRasterizer::LayerRasterizer() = default;

LayerRasterizer::~LayerRasterizer() {
    StopWorker();
}

void LayerRasterizer::BindDevice(ID2D1Device* device) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_device = device;
    m_workerContext.Reset();
    if (m_device) {
        m_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_workerContext);
    }
}

void LayerRasterizer::SetAsyncEnabled(bool enabled) {
    m_asyncEnabled = enabled;
    if (enabled) {
        EnsureWorker();
    }
}

void LayerRasterizer::Enqueue(RasterJob job) {
    if (!job) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_jobs.push_back(std::move(job));
}

bool LayerRasterizer::HasPendingJobs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_jobs.empty() || !m_workerJobs.empty();
}

int LayerRasterizer::FlushSync(GraphicsContext& ctx) {
    return FlushSync(ctx.GetD2DContext());
}

int LayerRasterizer::FlushSync(ID2D1DeviceContext* ctx) {
    std::vector<RasterJob> jobs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        jobs = std::move(m_jobs);
        m_jobs.clear();
    }
    int count = 0;
    for (auto& job : jobs) {
        if (job && ctx) {
            job(ctx);
            ++count;
        }
    }
    return count;
}

bool LayerRasterizer::KickAsync() {
    if (!m_asyncEnabled || !m_device || !m_workerContext) {
        return false;
    }
    EnsureWorker();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_jobs.empty()) {
            return true;
        }
        if (m_busy.load()) {
            return true; // already draining; new jobs wait for next kick
        }
        m_workerJobs = std::move(m_jobs);
        m_jobs.clear();
        m_kickRequested = true;
        m_busy.store(true);
    }
    m_cv.notify_one();
    return true;
}

void LayerRasterizer::EnsureWorker() {
    if (m_workerStarted) {
        return;
    }
    m_stop.store(false);
    m_worker = std::thread([this] { WorkerLoop(); });
    m_workerStarted = true;
}

void LayerRasterizer::StopWorker() {
    if (!m_workerStarted) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop.store(true);
        m_kickRequested = true;
    }
    m_cv.notify_all();
    if (m_worker.joinable()) {
        m_worker.join();
    }
    m_workerStarted = false;
    m_busy.store(false);
}

void LayerRasterizer::WorkerLoop() {
    for (;;) {
        std::vector<RasterJob> jobs;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] {
                return m_stop.load() || m_kickRequested;
            });
            if (m_stop.load() && m_workerJobs.empty() && !m_kickRequested) {
                break;
            }
            m_kickRequested = false;
            jobs = std::move(m_workerJobs);
            m_workerJobs.clear();
        }

        ID2D1DeviceContext* ctx = m_workerContext.Get();
        for (auto& job : jobs) {
            if (job && ctx) {
                job(ctx);
            }
        }

        m_busy.store(false);
        CompletionFn complete;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            complete = m_onComplete;
        }
        if (complete) {
            complete();
        }
        if (m_stop.load()) {
            break;
        }
    }
}

} // namespace CUI
