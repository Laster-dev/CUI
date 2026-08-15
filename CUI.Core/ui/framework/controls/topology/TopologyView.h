#pragma once

#include "../Control.h"
#include "TopologyModel.h"
#include "../ChromiumScrollAnimator.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace CUI {

/**
 * @brief 可折叠带平滑动画、子树隔离清晰排版与可编辑/只读切换的拓扑图控件。
 */
class TopologyView : public Control {
public:
    TopologyView();
    virtual ~TopologyView() override = default;

    virtual const char* GetClassName() const override { return "TopologyView"; }
    virtual HCURSOR GetCursor() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    // --- 数据与拓扑管理 ---
    void AddNode(std::shared_ptr<TopologyNode> node);
    void RemoveNode(const std::string& id);
    void AddEdge(TopologyEdge edge);
    void RemoveEdge(const std::string& fromId, const std::string& toId);
    void SetNodes(const std::vector<std::shared_ptr<TopologyNode>>& nodes);
    void SetEdges(const std::vector<TopologyEdge>& edges);
    void Clear();

    std::shared_ptr<TopologyNode> FindNode(const std::string& id) const;
    const std::vector<std::shared_ptr<TopologyNode>>& GetNodes() const { return m_nodes; }
    const std::vector<TopologyEdge>& GetEdges() const { return m_edges; }

    // --- 折叠与展开控制 ---
    void ExpandNode(const std::string& id);
    void CollapseNode(const std::string& id);
    void ToggleNode(const std::string& id);
    void ExpandAll();
    void CollapseAll();

    // --- 视口与排版控制 ---
    void SetLayoutType(TopologyLayoutType type);
    TopologyLayoutType GetLayoutType() const { return m_layoutType; }

    void Relayout();
    void FitToView(bool animated = true);
    void ResetView(bool animated = true);
    void ZoomIn(bool animated = true);
    void ZoomOut(bool animated = true);

    float GetZoom() const { return m_zoom; }
    void SetZoom(float zoom, bool animated = true);

    Point GetPanOffset() const { return m_panOffset; }
    void SetPanOffset(Point offset, bool animated = true);

    bool IsFlowParticlesEnabled() const { return m_flowParticlesEnabled; }
    void SetFlowParticlesEnabled(bool enabled);

    bool IsReadOnly() const { return m_isReadOnly; }
    void SetIsReadOnly(bool ro);

    std::shared_ptr<TopologyNode> GetSelectedItem() const { return m_selectedNode; }
    void SetSelectedItem(std::shared_ptr<TopologyNode> node);

    // --- 统一属性代理体系 ---
    /**
     * @brief 拓扑图节点集合属性代理。
     */
    struct TopologyNodesProperty {
        TopologyView* owner = nullptr;
        TopologyNodesProperty() = default;
        explicit TopologyNodesProperty(TopologyView* o) : owner(o) {}
        TopologyNodesProperty& operator=(const std::vector<std::shared_ptr<TopologyNode>>& nodes) { if (owner) owner->SetNodes(nodes); return *this; }
        operator const std::vector<std::shared_ptr<TopologyNode>>&() const { return owner->GetNodes(); }
        const std::vector<std::shared_ptr<TopologyNode>>& Get() const { return owner->GetNodes(); }
        size_t size() const { return owner->GetNodes().size(); }
        bool empty() const { return owner->GetNodes().empty(); }
    } Nodes;

    /**
     * @brief 拓扑图连线集合属性代理。
     */
    struct TopologyEdgesProperty {
        TopologyView* owner = nullptr;
        TopologyEdgesProperty() = default;
        explicit TopologyEdgesProperty(TopologyView* o) : owner(o) {}
        TopologyEdgesProperty& operator=(const std::vector<TopologyEdge>& edges) { if (owner) owner->SetEdges(edges); return *this; }
        operator const std::vector<TopologyEdge>&() const { return owner->GetEdges(); }
        const std::vector<TopologyEdge>& Get() const { return owner->GetEdges(); }
        size_t size() const { return owner->GetEdges().size(); }
        bool empty() const { return owner->GetEdges().empty(); }
    } Edges;

    /**
     * @brief 拓扑排版算法模式属性代理。
     */
    struct TopologyLayoutProperty {
        TopologyView* owner = nullptr;
        TopologyLayoutProperty() = default;
        explicit TopologyLayoutProperty(TopologyView* o) : owner(o) {}
        TopologyLayoutProperty& operator=(TopologyLayoutType t) { if (owner) owner->SetLayoutType(t); return *this; }
        operator TopologyLayoutType() const { return owner->GetLayoutType(); }
        TopologyLayoutType Get() const { return owner->GetLayoutType(); }
    } LayoutType;

    /**
     * @brief 当前选中的拓扑节点属性代理。
     */
    struct TopologySelectedItemProperty {
        TopologyView* owner = nullptr;
        TopologySelectedItemProperty() = default;
        explicit TopologySelectedItemProperty(TopologyView* o) : owner(o) {}
        TopologySelectedItemProperty& operator=(std::shared_ptr<TopologyNode> n) { if (owner) owner->SetSelectedItem(std::move(n)); return *this; }
        operator std::shared_ptr<TopologyNode>() const { return owner->GetSelectedItem(); }
        std::shared_ptr<TopologyNode> Get() const { return owner->GetSelectedItem(); }
        std::shared_ptr<TopologyNode> operator->() const { return owner->GetSelectedItem(); }
    } SelectedItem;

    /**
     * @brief 是否开启动态流动粒子特效属性代理。
     */
    struct TopologyFlowParticlesProperty {
        TopologyView* owner = nullptr;
        TopologyFlowParticlesProperty() = default;
        explicit TopologyFlowParticlesProperty(TopologyView* o) : owner(o) {}
        TopologyFlowParticlesProperty& operator=(bool e) { if (owner) owner->SetFlowParticlesEnabled(e); return *this; }
        operator bool() const { return owner->IsFlowParticlesEnabled(); }
        bool Get() const { return owner->IsFlowParticlesEnabled(); }
    } FlowParticles;

    /**
     * @brief 是否处于只读模式属性代理。
     */
    struct TopologyReadOnlyProperty {
        TopologyView* owner = nullptr;
        TopologyReadOnlyProperty() = default;
        explicit TopologyReadOnlyProperty(TopologyView* o) : owner(o) {}
        TopologyReadOnlyProperty& operator=(bool ro) { if (owner) owner->SetIsReadOnly(ro); return *this; }
        operator bool() const { return owner->IsReadOnly(); }
        bool Get() const { return owner->IsReadOnly(); }
    } ReadOnly;

    // --- 事件发布中心 ---
    Event<TopologyView*, std::shared_ptr<TopologyNode>>& OnNodeClicked() { return m_onNodeClicked; }
    Event<TopologyView*, std::shared_ptr<TopologyNode>>& OnNodeDoubleClicked() { return m_onNodeDoubleClicked; }
    Event<TopologyView*, std::shared_ptr<TopologyNode>, bool>& OnNodeToggleExpanded() { return m_onNodeToggleExpanded; }
    Event<TopologyView*, const TopologyEdge&>& OnEdgeClicked() { return m_onEdgeClicked; }
    Event<TopologyView*, std::shared_ptr<TopologyNode>>& OnSelectionChanged() { return m_onSelectionChanged; }

private:
    // --- 内部算法与渲染流程 ---
    void ComputeLayout();
    void ComputeHierarchicalLayout(bool horizontal);
    void ComputeRadialLayout();
    void StepForceDirectedSimulation();

    void UpdateCollapseVisibilityState();
    void CollectDescendants(const std::shared_ptr<TopologyNode>& parent, std::vector<std::shared_ptr<TopologyNode>>& outList);

    Point WorldToScreen(Point worldPt) const;
    Point ScreenToWorld(Point screenPt) const;
    Rect GetNodeBounds(const std::shared_ptr<TopologyNode>& node) const;
    Rect GetCollapseButtonBounds(const std::shared_ptr<TopologyNode>& node) const;

    std::shared_ptr<TopologyNode> HitTestNode(Point screenPt);
    std::shared_ptr<TopologyNode> HitTestCollapseButton(Point screenPt);
    int HitTestEdge(Point screenPt);

    void UpdateHoverHighlights(const std::shared_ptr<TopologyNode>& hoveredNode);

    // 绘制子模块
    void RenderGridBackground(GraphicsContext& ctx);
    void RenderEdges(GraphicsContext& ctx);
    void RenderNodes(GraphicsContext& ctx);
    void RenderMinimapAndToolbar(GraphicsContext& ctx);

    // 状态缓存
    std::vector<std::shared_ptr<TopologyNode>> m_nodes;
    std::vector<TopologyEdge> m_edges;
    std::unordered_map<std::string, std::shared_ptr<TopologyNode>> m_nodeMap;

    TopologyLayoutType m_layoutType = TopologyLayoutType::HierarchicalLeftRight;
    float m_zoom = 1.0f;
    Point m_panOffset{ 80.0f, 80.0f };
    ChromiumScrollAnimator m_zoomAnimator;
    ChromiumScrollAnimator m_panXAnimator;
    ChromiumScrollAnimator m_panYAnimator;
    bool m_flowParticlesEnabled = true;
    bool m_isReadOnly = false;

    // 交互操作变量
    bool m_isPanning = false;
    Point m_lastMousePos{ 0.0f, 0.0f };
    std::shared_ptr<TopologyNode> m_draggingNode = nullptr;
    std::shared_ptr<TopologyNode> m_hoveredNode = nullptr;
    std::shared_ptr<TopologyNode> m_selectedNode = nullptr;
    int m_hoveredEdgeIndex = -1;

    // 动画状态
    bool m_animating = false;
    float m_particleTick = 0.0f;
    float m_particleFrameAccumulator = 0.0f;

    // 事件
    Event<TopologyView*, std::shared_ptr<TopologyNode>> m_onNodeClicked;
    Event<TopologyView*, std::shared_ptr<TopologyNode>> m_onNodeDoubleClicked;
    Event<TopologyView*, std::shared_ptr<TopologyNode>, bool> m_onNodeToggleExpanded;
    Event<TopologyView*, const TopologyEdge&> m_onEdgeClicked;
    Event<TopologyView*, std::shared_ptr<TopologyNode>> m_onSelectionChanged;
};

} // namespace CUI
