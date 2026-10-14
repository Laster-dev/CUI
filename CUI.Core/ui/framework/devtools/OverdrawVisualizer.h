#pragma once

#include "DevToolsTypes.h"

namespace CUI {

/**
 * @brief 重绘区域与过度绘制实时热力图可视化器 (Overdraw Visualizer)
 */
class OverdrawVisualizer {
public:
    OverdrawVisualizer();

    /**
     * @brief 记录本帧发生重绘的脏矩形区域
     */
    void RecordDirtyRects(const std::vector<Rect>& rects);

    /**
     * @brief 在当前帧顶层绘制热力图色彩遮罩与指示图例
     */
    void Render(GraphicsContext& ctx, Size windowSize);

    /**
     * @brief 清空当前记录的重绘脏区
     */
    void Clear();

private:
    struct TrackedOverdrawRect {
        Rect bounds;
        std::chrono::steady_clock::time_point timestamp;
        int hitCount = 1;
    };

    std::vector<TrackedOverdrawRect> m_history;
};

} // namespace CUI
