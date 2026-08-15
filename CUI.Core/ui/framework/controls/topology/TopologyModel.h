#pragma once

#include "../UIElement.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <unordered_map>
#include <d2d1.h>

namespace CUI {

/**
 * @brief 拓扑节点运行与健康状态枚举。
 */
enum class TopologyNodeStatus : uint8_t {
    Normal = 0,    ///< 正常/就绪状态 (蓝色/主题色)
    Success,       ///< 健康/正常运行中 (绿色)
    Warning,       ///< 告警/轻微异常 (橙黄色)
    Error,         ///< 故障/严重错误 (红色)
    Inactive,      ///< 未激活/离线 (灰色)
    Processing     ///< 执行中/高负载 (动态呼吸蓝紫)
};

/**
 * @brief 拓扑图自动布局算法类型枚举。
 */
enum class TopologyLayoutType : uint8_t {
    HierarchicalLeftRight = 0, ///< 层次化分明排版 (从左到右，按子树隔离不交织)
    HierarchicalTopDown,       ///< 层次化树状排版 (从上到下)
    Radial,                    ///< 同心圆/径向发散布局
    ForceDirected              ///< 弹簧力导向有机布局
};

/**
 * @brief 拓扑连线线条渲染样式。
 */
enum class TopologyEdgeStyle : uint8_t {
    Solid = 0,   ///< 实线
    Dashed,      ///< 虚线
    Dotted       ///< 点线
};

/**
 * @brief 拓扑图节点数据结构。
 */
struct TopologyNode : public std::enable_shared_from_this<TopologyNode> {
    std::string id;                                     ///< 节点唯一标识键
    std::string label;                                  ///< 节点主标题文本
    std::string subLabel;                               ///< 节点副标题/类型说明文本
    std::string icon;                                   ///< 节点前缀图标或符号 (Unicode/Emoji)
    std::string badge;                                  ///< 右上角徽标指标文本 (如 "120 req/s", "3 Nodes")
    std::string category;                               ///< 分组类别 (如 "Gateway", "Service", "Database")
    TopologyNodeStatus status = TopologyNodeStatus::Normal; ///< 当前状态

    bool isExpanded = true;                             ///< 子图是否处于展开状态
    bool isCollapsible = false;                         ///< 是否具备可折叠子分支的能力
    std::string parentId;                               ///< 父节点标识符 (用于树状层级折叠)
    std::vector<std::shared_ptr<TopologyNode>> children;///< 直系子节点集合

    std::optional<D2D1_COLOR_F> customColor;            ///< 节点自定义强调主色

    // --- 动态排版与动画渲染参数 ---
    Point currentPos{ 0.0f, 0.0f };                     ///< 当前动画插值渲染坐标
    Point targetPos{ 0.0f, 0.0f };                      ///< 目标排版坐标
    Point velocity{ 0.0f, 0.0f };                       ///< 力导向速度矢量
    float opacity = 1.0f;                               ///< 当前渲染透明度 (0.0~1.0)
    float targetOpacity = 1.0f;                         ///< 目标透明度
    float scale = 1.0f;                                 ///< 当前缩放比例 (0.0~1.0)
    float targetScale = 1.0f;                           ///< 目标缩放比例
    Size size{ 170.0f, 66.0f };                         ///< 节点卡片尺寸
    
    // --- 交互状态 ---
    bool isHovered = false;                             ///< 鼠标是否悬停于节点上方
    bool isSelected = false;                            ///< 节点是否被选中
    bool isHighlighted = false;                         ///< 是否处于关联路径高亮激活态
    bool isDimmed = false;                              ///< 是否因其他节点悬停而淡化背景
    bool isCulled = false;                              ///< 是否因父级折叠完全隐去并停止交互
    bool isDragging = false;                            ///< 是否正在被鼠标按住拖拽移动

    TopologyNode() = default;
    TopologyNode(std::string nodeId, std::string title, std::string subTitle = "", std::string iconSymbol = "")
        : id(std::move(nodeId)), label(std::move(title)), subLabel(std::move(subTitle)), icon(std::move(iconSymbol)) {}

    /**
     * @brief 添加一个子节点并自动建立父子关系与可折叠标识。
     */
    void AddChild(std::shared_ptr<TopologyNode> child) {
        if (!child) return;
        child->parentId = this->id;
        children.push_back(child);
        isCollapsible = true;
    }
};

/**
 * @brief 拓扑图节点之间的连接关系连线。
 */
struct TopologyEdge {
    std::string id;                                     ///< 连线唯一标识
    std::string fromId;                                 ///< 起点节点标识
    std::string toId;                                   ///< 终点节点标识
    std::string label;                                  ///< 连线标注文本 (如 "HTTPS", "Kafka")
    TopologyEdgeStyle style = TopologyEdgeStyle::Solid; ///< 连线线型
    std::optional<D2D1_COLOR_F> customColor;            ///< 自定义连线颜色
    bool isDirected = true;                             ///< 是否具有方向箭头
    
    // --- 动态流向粒子特效 ---
    bool isFlowActive = true;                           ///< 是否开启动态数据流动粒子
    float flowSpeed = 1.0f;                             ///< 粒子移动速率
    float flowPhase = 0.0f;                             ///< 当前流动相位 [0, 1)

    // --- 渲染交互控制 ---
    float opacity = 1.0f;                               ///< 连线透明度
    float targetOpacity = 1.0f;                         ///< 目标透明度
    bool isHighlighted = false;                         ///< 是否高亮激活
    bool isDimmed = false;                              ///< 是否淡化
    bool isCulled = false;                              ///< 是否因端点折叠完全隐去

    TopologyEdge() = default;
    TopologyEdge(std::string fromNode, std::string toNode, std::string edgeLabel = "", bool directed = true)
        : fromId(std::move(fromNode)), toId(std::move(toNode)), label(std::move(edgeLabel)), isDirected(directed) {
        id = fromId + "->" + toId;
    }
};

} // namespace CUI
