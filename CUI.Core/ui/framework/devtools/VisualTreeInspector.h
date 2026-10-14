#pragma once

#include "DevToolsTypes.h"
#include "BoxModelOverlay.h"

namespace CUI {

/**
 * @brief 现代运行时可视元素树与属性检查面板 (In-App Visual Tree Inspector)
 */
class VisualTreeInspector {
public:
    VisualTreeInspector();

    /**
     * @brief 刷新重建当前窗口的可视化树拓扑结构
     */
    void RebuildTree(UIElement* root);

    /**
     * @brief 获取当前选中的控件指针
     */
    UIElement* GetSelectedElement() const { return m_selectedElement; }

    /**
     * @brief 选中指定控件并同步在树中高亮定位
     */
    void SelectElement(UIElement* element, bool forceCenter = false);

    /**
     * @brief 滚动树视图以使指定控件进入视野
     */
    void ScrollToElement(UIElement* element, bool forceCenter = false);

    /**
     * @brief 鼠标移动处理 (面板拖拽、悬停检测)
     */
    bool OnMouseMove(Point pt);

    /**
     * @brief 鼠标按下处理 (标题栏拖拽、工具栏点击、树节点展开折叠与选中)
     */
    bool OnMouseDown(Point pt);

    /**
     * @brief 鼠标抬起处理
     */
    bool OnMouseUp(Point pt);

    /**
     * @brief 滚轮滚动处理 (树节点列表纵向滚动)
     */
    bool OnMouseWheel(Point pt, float delta);

    /**
     * @brief 检查面板是否包含某坐标 (用于命中判定)
     */
    bool ContainsPoint(Point pt) const { return m_panelRect.Contains(pt.x, pt.y); }

    /**
     * @brief 获取面板绘制区域
     */
    Rect GetBounds() const { return m_panelRect; }

    /**
     * @brief 渲染整个 DevTools 检查面板
     */
    void Render(GraphicsContext& ctx, Size windowSize);

private:
    struct FlatTreeNode {
        std::shared_ptr<VisualTreeNode> node;
        int depth = 0;
        float y = 0.0f;
        float height = 24.0f;
    };

    void BuildNodeRecursive(UIElement* el, std::shared_ptr<VisualTreeNode> parentNode, int depth);
    void FlattenTree();

    Rect m_panelRect{ 80.0f, 60.0f, 740.0f, 520.0f };
    bool m_isDragging = false;
    Point m_dragOffset{};

    float m_treeScrollY = 0.0f;
    float m_maxTreeScroll = 0.0f;
    bool m_isDraggingScrollbar = false;
    float m_dragScrollStartY = 0.0f;
    float m_dragScrollStartVal = 0.0f;
    bool m_hideCollapsed = false;

    UIElement* m_selectedElement = nullptr;
    std::shared_ptr<VisualTreeNode> m_treeRoot;
    std::vector<FlatTreeNode> m_flatNodes;

    // 缓存展开状态 (按元素原始地址映射)
    std::unordered_map<const UIElement*, bool> m_expandedStates;
};

} // namespace CUI
