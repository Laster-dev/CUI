#include "Compositor.h"
#include "GraphicsContext.h"
#include <numeric>

namespace CUI {

Compositor::Compositor() {
    m_lastFrameTimestamp = std::chrono::high_resolution_clock::now();
}

Compositor::~Compositor() {
    Shutdown();
}

void Compositor::Initialize(GraphicsContext* ctx, HWND hwnd) {
    m_gfxContext = ctx;
    m_hwnd = hwnd;
    m_lastFrameTimestamp = std::chrono::high_resolution_clock::now();
}

void Compositor::Shutdown() {
    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_pendingFrame = nullptr;
    m_gfxContext = nullptr;
    m_hwnd = nullptr;
}

void Compositor::SubmitFrame(std::shared_ptr<CompositorFrame> frame) {
    if (!frame) return;

    if (m_enableOptimization) {
        if (frame->backgroundList) frame->backgroundList->Optimize();
        if (frame->sceneList) frame->sceneList->Optimize();
        if (frame->overlayList) frame->overlayList->Optimize();
    }

    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_pendingFrame = std::move(frame);
}

bool Compositor::RenderAndPresent() {
    std::shared_ptr<CompositorFrame> frame;
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        if (!m_pendingFrame) return false;
        frame = std::move(m_pendingFrame);
    }

    if (!m_gfxContext) return false;

    const auto startTime = std::chrono::high_resolution_clock::now();

    m_gfxContext->BeginDraw();

    if (frame->clearColor.a > 0.001f) {
        m_gfxContext->ClearRect(frame->viewportBounds, frame->clearColor);
    }

    size_t totalOps = 0;
    size_t totalSavings = 0;

    // 1. 绘制背景层
    if (frame->backgroundList) {
        totalOps += frame->backgroundList->GetOpCount();
        totalSavings += frame->backgroundList->GetBatchReductionCount();
        frame->backgroundList->Playback(*m_gfxContext);
    }

    // 2. 绘制主场景层
    if (frame->sceneList) {
        totalOps += frame->sceneList->GetOpCount();
        totalSavings += frame->sceneList->GetBatchReductionCount();
        frame->sceneList->Playback(*m_gfxContext);
    }

    // 3. 绘制覆盖/悬浮层
    if (frame->overlayList) {
        totalOps += frame->overlayList->GetOpCount();
        totalSavings += frame->overlayList->GetBatchReductionCount();
        frame->overlayList->Playback(*m_gfxContext);
    }

    m_gfxContext->EndDraw();

    const auto endTime = std::chrono::high_resolution_clock::now();
    const float frameDurationMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    // 更新诊断指标
    ++m_diagnostics.totalFramesRendered;
    m_diagnostics.lastFrameGpuTimeMs = frameDurationMs;
    m_diagnostics.totalOpsInLastFrame = totalOps;
    m_diagnostics.batchSavingsInLastFrame = totalSavings;

    const float deltaSec = std::chrono::duration<float>(endTime - m_lastFrameTimestamp).count();
    m_lastFrameTimestamp = endTime;

    if (deltaSec > 0.0001f) {
        const float instantFps = 1.0f / deltaSec;
        m_recentFrameDurationsMs.push_back(instantFps);
        if (m_recentFrameDurationsMs.size() > 30) {
            m_recentFrameDurationsMs.erase(m_recentFrameDurationsMs.begin());
        }
        float sumFps = std::accumulate(m_recentFrameDurationsMs.begin(), m_recentFrameDurationsMs.end(), 0.0f);
        m_diagnostics.currentFps = sumFps / static_cast<float>(m_recentFrameDurationsMs.size());
    }

    return true;
}

void Compositor::ResetDiagnostics() {
    m_diagnostics = CompositorDiagnostics();
    m_recentFrameDurationsMs.clear();
    m_lastFrameTimestamp = std::chrono::high_resolution_clock::now();
}

} // namespace CUI
