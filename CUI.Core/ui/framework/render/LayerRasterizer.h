#pragma once
#include <windows.h>
#include <unknwn.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <functional>
#include <mutex>
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <wrl/client.h>

namespace CUI {

using Microsoft::WRL::ComPtr;
class GraphicsContext;

// Rasterizes dirty PictureLayers. Sync flush is the default safe path.
// When async is enabled and a D2D device is bound, jobs run on a worker with a
// dedicated ID2D1DeviceContext (ID3D11Multithread must be on). UI-tree OnRender
// must NOT be called from worker jobs — only bitmap/layer fills.
class LayerRasterizer {
public:
    using RasterJob = std::function<void(ID2D1DeviceContext*)>;
    using CompletionFn = std::function<void()>;

    LayerRasterizer();
    ~LayerRasterizer();

    LayerRasterizer(const LayerRasterizer&) = delete;
    LayerRasterizer& operator=(const LayerRasterizer&) = delete;

    void BindDevice(ID2D1Device* device);
    void SetCompletionCallback(CompletionFn fn) { m_onComplete = std::move(fn); }

    void Enqueue(RasterJob job);
    // Drain on the calling thread (UI). Returns number executed.
    int FlushSync(GraphicsContext& ctx);
    int FlushSync(ID2D1DeviceContext* ctx);

    // Queue current jobs to the worker. Returns false if async unavailable (caller should FlushSync).
    bool KickAsync();
    bool IsBusy() const { return m_busy.load(); }
    bool HasPendingJobs() const;

    void SetAsyncEnabled(bool enabled);
    bool IsAsyncEnabled() const { return m_asyncEnabled; }

private:
    void EnsureWorker();
    void StopWorker();
    void WorkerLoop();

    mutable std::mutex m_mutex;
    std::vector<RasterJob> m_jobs;
    std::vector<RasterJob> m_workerJobs;
    CompletionFn m_onComplete;

    ComPtr<ID2D1Device> m_device;
    ComPtr<ID2D1DeviceContext> m_workerContext;

    std::thread m_worker;
    std::condition_variable m_cv;
    std::atomic<bool> m_stop{ false };
    std::atomic<bool> m_busy{ false };
    bool m_workerStarted = false;
    bool m_asyncEnabled = false;
    bool m_kickRequested = false;
};

} // namespace CUI
