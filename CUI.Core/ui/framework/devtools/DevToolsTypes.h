#pragma once

#include "../controls/UIElement.h"
#include "../render/GraphicsContext.h"
#include "../style/ThemeManager.h"
#include "../style/ThemeTokenId.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace CUI {

/**
 * @brief 盒模型四个核心同心测量矩形及度量数据
 */
struct BoxModelMetrics {
    Rect bounds{};             // 控件自身物理边界 (无外边距)
    Thickness margin{};        // 外边距
    Thickness padding{};       // 内边距
    float borderThickness = 0.0f; // 边框粗细

    Rect marginRect{};         // 外边距外包围矩形
    Rect borderRect{};         // 边框矩形 (= bounds)
    Rect paddingRect{};        // 剔除边框后的内边距矩形
    Rect contentRect{};        // 剔除边框和内边距后的纯内容矩形

    std::string typeName;      // 控件类型名 (e.g. Button, VirtualizingStackPanel)
    std::string elementAddress;// 实例内存指针地址字符串
    Visibility visibility = Visibility::Visible;
    bool isEnabled = true;
    bool isFocused = false;
    size_t childCount = 0;
};

/**
 * @brief 实时性能诊断与帧率指标数据
 */
struct DevToolsProfilerStats {
    float fps = 0.0f;                   // 实时平滑帧率
    float frameTimeMs = 0.0f;           // 当前帧耗时 (毫秒)
    float layoutDurationMs = 0.0f;      // 当前帧布局测算耗时 (毫秒)
    float renderDurationMs = 0.0f;      // 当前帧绘制与光栅化耗时 (毫秒)
    uint32_t totalDrawOps = 0;          // 本帧 DrawOp 指令总数
    uint32_t batchReductions = 0;       // DisplayList 批处理优化消除指令数
    uint32_t layerCacheHits = 0;        // 图层缓存命中数
    size_t estimatedMemoryBytes = 0;    // 估算渲染内存开销 (字节)
    std::vector<float> frameTimeHistory;// 最近 60 帧耗时历史 (用于绘制 mini 折线走势图)
};

/**
 * @brief 运行时可视树节点信息
 */
struct VisualTreeNode {
    UIElement* rawElement = nullptr;
    std::string typeName;
    std::string summary;
    int depth = 0;
    bool isExpanded = true;
    bool isSelected = false;
    bool isHovered = false;
    Rect bounds{};
    Visibility visibility = Visibility::Visible;
    std::vector<std::shared_ptr<VisualTreeNode>> children;
};

/**
 * @brief 盒模型标准调试着色规范 (与现代浏览器 DevTools 一致)
 */
namespace DevToolsColors {
    // 外边距: 温暖橙色 (半透明)
    inline Color Margin() { return Color(0.96f, 0.62f, 0.28f, 0.45f); }
    inline Color MarginStroke() { return Color(0.96f, 0.62f, 0.28f, 0.90f); }

    // 边框: 鹅黄色 (半透明)
    inline Color Border() { return Color(0.99f, 0.88f, 0.55f, 0.45f); }
    inline Color BorderStroke() { return Color(0.99f, 0.88f, 0.55f, 0.90f); }

    // 内边距: 草绿色 (半透明)
    inline Color Padding() { return Color(0.55f, 0.77f, 0.35f, 0.45f); }
    inline Color PaddingStroke() { return Color(0.55f, 0.77f, 0.35f, 0.90f); }

    // 内容区: 天蓝色 (半透明)
    inline Color Content() { return Color(0.36f, 0.68f, 0.89f, 0.45f); }
    inline Color ContentStroke() { return Color(0.36f, 0.68f, 0.89f, 0.90f); }

    // 悬浮聚焦高亮卡片背景与描边
    inline Color BadgeBackground() { return Color(0.11f, 0.13f, 0.16f, 0.92f); }
    inline Color BadgeBorder() { return Color(0.35f, 0.40f, 0.48f, 0.75f); }
    inline Color BadgeText() { return Color(0.95f, 0.96f, 0.98f, 1.0f); }
    inline Color BadgeAccent() { return Color(0.38f, 0.76f, 0.99f, 1.0f); }
}

/**
 * @brief 辅助解析元素类型名 (自动去除 CUI:: 与 class 等修饰符)
 */
inline std::string ExtractElementTypeName(const UIElement* el) {
    if (!el) return "Null";
    const char* cname = el->GetClassName();
    if (cname && std::strcmp(cname, "Object") != 0 && std::strcmp(cname, "UIElement") != 0) {
        return cname;
    }
    std::string raw = typeid(*el).name();
    const char* prefixes[] = { "class ", "struct ", "CUI::", "Widgets::", "Fluent::" };
    for (const char* p : prefixes) {
        size_t pos = 0;
        const size_t plen = std::strlen(p);
        while ((pos = raw.find(p, pos)) != std::string::npos) {
            raw.erase(pos, plen);
        }
    }
    return raw;
}

} // namespace CUI
