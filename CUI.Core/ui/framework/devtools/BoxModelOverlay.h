#pragma once

#include "DevToolsTypes.h"

namespace CUI {

/**
 * @brief 现代盒模型诊断遮罩渲染器 (Chrome DevTools 风格)
 */
class BoxModelOverlay {
public:
    /**
     * @brief 提取目标控件的完整盒模型几何指标
     */
    static BoxModelMetrics ExtractMetrics(const UIElement* element);

    /**
     * @brief 在画布顶层渲染目标控件的高亮诊断遮罩与悬浮信息气泡
     * @param ctx 绘图上下文
     * @param element 目标控件指针
     * @param windowSize 当前窗口可用逻辑视口尺寸
     */
    static void RenderOverlay(GraphicsContext& ctx, const UIElement* element, Size windowSize);

    /**
     * @brief 在 DevTools 检查面板中渲染嵌套的同心盒模型架构图
     * @param ctx 绘图上下文
     * @param metrics 盒模型指标
     * @param destRect 绘制目标矩形区域
     */
    static void RenderDiagram(GraphicsContext& ctx, const BoxModelMetrics& metrics, Rect destRect);
};

} // namespace CUI
