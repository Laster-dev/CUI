#pragma once

#include "DevToolsTypes.h"

namespace CUI {

/**
 * @brief 现代实时渲染性能与帧率仪表盘 (Performance & FPS Overlay)
 */
class PerformanceOverlay {
public:
    PerformanceOverlay();

    /**
     * @brief 记录并压入一帧的新采样指标
     */
    void RecordFrame(float frameTimeMs, float layoutMs, float renderMs,
                     uint32_t drawOps = 0, uint32_t batchReductions = 0, uint32_t layerCacheHits = 0);

    /**
     * @brief 获取当前累积统计与历史数据
     */
    const DevToolsProfilerStats& GetStats() const { return m_stats; }

    /**
     * @brief 绘制悬浮 HUD 仪表盘
     * @param ctx 绘图上下文
     * @param windowSize 当前逻辑窗口尺寸
     * @param anchorPos 自定义停靠坐标 (若为 (-1, -1) 则自动右上角避让悬浮)
     */
    void Render(GraphicsContext& ctx, Size windowSize, Point anchorPos = { -1.0f, -1.0f });

private:
    DevToolsProfilerStats m_stats;
    std::chrono::steady_clock::time_point m_fpsSampleStart{};
    uint32_t m_fpsFrameCount = 0;
    static constexpr size_t kMaxHistory = 60;
};

} // namespace CUI
