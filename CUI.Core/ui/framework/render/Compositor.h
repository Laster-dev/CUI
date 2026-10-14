#pragma once
#include <memory>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include "DisplayList.h"
#include "DirtyRegion.h"

namespace CUI {

class GraphicsContext;

/**
 * @brief 合成帧数据包 (CompositorFrame)
 * 封装单帧待呈现的所有渲染指令流与视口信息。
 */
struct CompositorFrame {
    uint64_t frameId = 0;
    Rect viewportBounds;
    DirtyRegion dirtyRegion;
    Color clearColor = Color::Transparent;
    bool fullRepaint = false;

    std::shared_ptr<DisplayList> backgroundList;
    std::shared_ptr<DisplayList> sceneList;
    std::shared_ptr<DisplayList> overlayList;

    std::chrono::high_resolution_clock::time_point timestamp;
};

/**
 * @brief 合成器运行时诊断指标
 */
struct CompositorDiagnostics {
    uint64_t totalFramesRendered = 0;
    float currentFps = 60.0f;
    float lastFrameCpuTimeMs = 0.0f;
    float lastFrameGpuTimeMs = 0.0f;
    size_t totalOpsInLastFrame = 0;
    size_t batchSavingsInLastFrame = 0;
};

/**
 * @brief 现代合成器 (Compositor)
 * 协调 DisplayList 指令流提交、视口裁剪、GPU 批处理与呈现管线。
 */
class Compositor {
public:
    Compositor();
    ~Compositor();

    void Initialize(GraphicsContext* ctx, HWND hwnd);
    void Shutdown();

    // 提交合成帧
    void SubmitFrame(std::shared_ptr<CompositorFrame> frame);

    // 立即执行当前待呈现帧的光栅化与 Present
    bool RenderAndPresent();

    // 诊断与性能度量
    const CompositorDiagnostics& GetDiagnostics() const { return m_diagnostics; }
    void ResetDiagnostics();

    // 优化选项
    void EnableDisplayListOptimization(bool enable) { m_enableOptimization = enable; }
    bool IsDisplayListOptimizationEnabled() const { return m_enableOptimization; }

private:
    GraphicsContext* m_gfxContext = nullptr;
    HWND m_hwnd = nullptr;
    std::shared_ptr<CompositorFrame> m_pendingFrame;
    std::mutex m_frameMutex;

    bool m_enableOptimization = true;
    CompositorDiagnostics m_diagnostics;

    std::chrono::high_resolution_clock::time_point m_lastFrameTimestamp;
    std::vector<float> m_recentFrameDurationsMs;
};

} // namespace CUI
