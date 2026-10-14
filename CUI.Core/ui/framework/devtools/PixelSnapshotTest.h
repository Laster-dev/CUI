#pragma once

#include "../controls/UIElement.h"
#include "../render/GraphicsContext.h"

#include <cstdint>
#include <string>
#include <vector>

namespace CUI {

/**
 * @brief 离线/运行时图像像素快照数据
 */
struct SnapshotBuffer {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint32_t> pixels; // 0xAARRGGBB 格式像素阵列
};

/**
 * @brief 像素快照比对与回归测试报告 (Golden Image Diff Report)
 */
struct SnapshotDiffReport {
    bool passed = true;             // 是否通过回归校验
    uint32_t totalPixels = 0;       // 采样像素总量
    uint32_t diffPixels = 0;        // 超出容差的差异像素数
    float diffRatio = 0.0f;         // 差异像素百分比 (0.0 - 1.0)
    float maxDelta = 0.0f;          // 单像素最大通道色彩偏差 (0.0 - 1.0)
    SnapshotBuffer diffHeatmap;     // 差异像素热力对比图
};

/**
 * @brief 自动化像素快照与基准图像回归测试套件 (Golden Image Testing)
 */
class PixelSnapshotTest {
public:
    /**
     * @brief 为指定 UI 元素生成离屏光栅化像素快照
     * @param element 目标控件指针
     * @param targetSize 快照逻辑尺寸
     * @param ctx 图形上下文
     */
    static SnapshotBuffer CaptureElement(UIElement* element, Size targetSize, GraphicsContext& ctx);

    /**
     * @brief 逐像素比对基准快照 (Golden) 与待测快照 (Candidate)
     * @param golden 基准快照
     * @param candidate 待测候选快照
     * @param tolerancePerChannel 单通道容差阈值 (0.0 - 1.0, 默认 0.03f 滤除亚像素抗锯齿抖动)
     * @param maxAllowedDiffRatio 允许的最大差异像素比率 (默认 0.001f 即 0.1%)
     */
    static SnapshotDiffReport Compare(
        const SnapshotBuffer& golden,
        const SnapshotBuffer& candidate,
        float tolerancePerChannel = 0.03f,
        float maxAllowedDiffRatio = 0.001f);

    /**
     * @brief 将像素快照保存为本地 PNG 文件
     */
    static bool SaveToPng(const SnapshotBuffer& buffer, const std::wstring& filePath, GraphicsContext& ctx);
};

} // namespace CUI
