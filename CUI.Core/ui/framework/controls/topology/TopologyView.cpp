#define NOMINMAX
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "TopologyView.h"
#include "../../render/GraphicsContext.h"
#include "../../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <format>
#include <unordered_set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CUI {

TopologyView::TopologyView()
    : Nodes(this)
    , Edges(this)
    , LayoutType(this)
    , SelectedItem(this)
    , FlowParticles(this)
    , ReadOnly(this)
{
    SetWidth(-1.0f);
    SetHeight(560.0f);
    m_zoomAnimator.Reset(1.0f);
    m_panXAnimator.Reset(60.0f);
    m_panYAnimator.Reset(60.0f);
    m_zoom = 1.0f;
    m_panOffset = Point(60.0f, 60.0f);
}

HCURSOR TopologyView::GetCursor() const {
    if (m_isPanning) return LoadCursor(nullptr, IDC_SIZEALL);
    if (!m_isReadOnly && m_draggingNode) return LoadCursor(nullptr, IDC_HAND);
    if (m_hoveredNode) return LoadCursor(nullptr, IDC_HAND);
    return LoadCursor(nullptr, IDC_ARROW);
}

Size TopologyView::Measure(Size availableSize) {
    float expW = GetWidth();
    if (expW < 0.0f) {
        expW = (availableSize.width > 0.0f && availableSize.width < 10000.0f) ? availableSize.width : 860.0f;
    }
    float expH = GetHeight();
    if (expH < 0.0f) {
        expH = 560.0f;
    }
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void TopologyView::Arrange(Rect finalRect) {
    SetBounds(finalRect);
}

void TopologyView::AddNode(std::shared_ptr<TopologyNode> node) {
    if (!node || node->id.empty()) return;
    if (m_nodeMap.find(node->id) == m_nodeMap.end()) {
        m_nodes.push_back(node);
        m_nodeMap[node->id] = node;
        Relayout();
    }
}

void TopologyView::RemoveNode(const std::string& id) {
    if (id.empty()) return;
    auto rootIt = m_nodeMap.find(id);
    if (rootIt == m_nodeMap.end()) return;

    std::unordered_set<std::string> removedIds;
    std::vector<std::shared_ptr<TopologyNode>> pending{ rootIt->second };
    while (!pending.empty()) {
        auto node = pending.back();
        pending.pop_back();
        if (!node || node->id.empty() || !removedIds.insert(node->id).second) continue;
        for (const auto& child : node->children) if (child) pending.push_back(child);
        for (const auto& candidate : m_nodes) {
            if (candidate && candidate->parentId == node->id) pending.push_back(candidate);
        }
    }

    const auto isRemoved = [&removedIds](const std::shared_ptr<TopologyNode>& node) {
        return !node || removedIds.contains(node->id);
    };
    if (m_selectedNode && removedIds.contains(m_selectedNode->id)) SetSelectedItem(nullptr);
    if (m_hoveredNode && removedIds.contains(m_hoveredNode->id)) m_hoveredNode = nullptr;
    if (m_draggingNode && removedIds.contains(m_draggingNode->id)) {
        m_draggingNode->isDragging = false;
        m_draggingNode = nullptr;
    }

    m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(), isRemoved), m_nodes.end());
    for (auto& node : m_nodes) {
        if (!node) continue;
        node->children.erase(std::remove_if(node->children.begin(), node->children.end(), isRemoved), node->children.end());
        node->isCollapsible = !node->children.empty();
    }
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(), [&removedIds](const TopologyEdge& edge) {
        return removedIds.contains(edge.fromId) || removedIds.contains(edge.toId);
    }), m_edges.end());
    for (const auto& removedId : removedIds) m_nodeMap.erase(removedId);
    Relayout();
}

void TopologyView::AddEdge(TopologyEdge edge) {
    m_edges.push_back(edge);
    Relayout();
}

void TopologyView::RemoveEdge(const std::string& fromId, const std::string& toId) {
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(), [&](const TopologyEdge& e) {
        return e.fromId == fromId && e.toId == toId;
    }), m_edges.end());
    Relayout();
}

void TopologyView::SetNodes(const std::vector<std::shared_ptr<TopologyNode>>& nodes) {
    m_nodes = nodes;
    m_nodeMap.clear();
    for (const auto& n : m_nodes) {
        if (n) m_nodeMap[n->id] = n;
    }
    Relayout();
}

void TopologyView::SetEdges(const std::vector<TopologyEdge>& edges) {
    m_edges = edges;
    Relayout();
}

void TopologyView::Clear() {
    m_nodes.clear();
    m_edges.clear();
    m_nodeMap.clear();
    m_selectedNode = nullptr;
    m_hoveredNode = nullptr;
    m_draggingNode = nullptr;
    MarkRenderRectDirty(GetBounds());
}

std::shared_ptr<TopologyNode> TopologyView::FindNode(const std::string& id) const {
    auto it = m_nodeMap.find(id);
    return (it != m_nodeMap.end()) ? it->second : nullptr;
}

void TopologyView::SetLayoutType(TopologyLayoutType type) {
    if (m_layoutType != type) {
        m_layoutType = type;
        Relayout();
    }
}

void TopologyView::SetIsReadOnly(bool ro) {
    if (m_isReadOnly != ro) {
        m_isReadOnly = ro;
        if (m_draggingNode) {
            m_draggingNode->isDragging = false;
            m_draggingNode = nullptr;
        }
        MarkRenderRectDirty(GetBounds());
    }
}

void TopologyView::SetZoom(float zoom, bool animated) {
    float clamped = (std::clamp)(zoom, 0.2f, 3.5f);
    if (animated && UIElement::AreAnimationsEnabled()) {
        m_zoomAnimator.ScrollBy(clamped - m_zoomAnimator.Target(), 0.2f, 3.5f);
        m_animating = true;
        RequestAnimationTicks();
    } else {
        m_zoomAnimator.JumpTo(clamped);
        m_zoom = clamped;
        MarkRenderRectDirty(GetBounds());
    }
}

void TopologyView::SetPanOffset(Point offset, bool animated) {
    if (animated && UIElement::AreAnimationsEnabled()) {
        m_panXAnimator.ScrollBy(offset.x - m_panXAnimator.Target(), -50000.0f, 50000.0f);
        m_panYAnimator.ScrollBy(offset.y - m_panYAnimator.Target(), -50000.0f, 50000.0f);
        m_animating = true;
        RequestAnimationTicks();
    } else {
        m_panXAnimator.JumpTo(offset.x);
        m_panYAnimator.JumpTo(offset.y);
        m_panOffset = offset;
        MarkRenderRectDirty(GetBounds());
    }
}

void TopologyView::ZoomIn(bool animated) {
    float curTarget = m_zoomAnimator.Target();
    SetZoom(curTarget * 1.25f, animated);
}

void TopologyView::ZoomOut(bool animated) {
    float curTarget = m_zoomAnimator.Target();
    SetZoom(curTarget / 1.25f, animated);
}

void TopologyView::SetFlowParticlesEnabled(bool enabled) {
    m_flowParticlesEnabled = enabled;
    if (enabled) RequestAnimationTicks();
    MarkRenderContentDirty();
}

void TopologyView::SetSelectedItem(std::shared_ptr<TopologyNode> node) {
    if (m_selectedNode != node) {
        if (m_selectedNode) m_selectedNode->isSelected = false;
        m_selectedNode = node;
        if (m_selectedNode) m_selectedNode->isSelected = true;
        m_onSelectionChanged.Invoke(this, m_selectedNode);
        MarkRenderContentDirty();
    }
}

void TopologyView::ExpandNode(const std::string& id) {
    auto node = FindNode(id);
    if (node && !node->isExpanded) {
        node->isExpanded = true;
        m_onNodeToggleExpanded.Invoke(this, node, true);
        Relayout();
    }
}

void TopologyView::CollapseNode(const std::string& id) {
    auto node = FindNode(id);
    if (node && node->isExpanded) {
        node->isExpanded = false;
        m_onNodeToggleExpanded.Invoke(this, node, false);
        Relayout();
    }
}

void TopologyView::ToggleNode(const std::string& id) {
    auto node = FindNode(id);
    if (node) {
        if (node->isExpanded) CollapseNode(id);
        else ExpandNode(id);
    }
}

void TopologyView::ExpandAll() {
    for (auto& n : m_nodes) {
        if (n && n->isCollapsible) n->isExpanded = true;
    }
    Relayout();
}

void TopologyView::CollapseAll() {
    for (auto& n : m_nodes) {
        if (n && n->isCollapsible) n->isExpanded = false;
    }
    Relayout();
}

void TopologyView::ResetView(bool animated) {
    SetZoom(1.0f, animated);
    SetPanOffset(Point(60.0f, 60.0f), animated);
    Relayout();
}

void TopologyView::FitToView(bool animated) {
    if (m_nodes.empty()) return;

    float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
    int visibleCount = 0;
    for (const auto& n : m_nodes) {
        if (n && !n->isCulled && n->targetOpacity > 0.05f) {
            minX = (std::min)(minX, n->targetPos.x);
            minY = (std::min)(minY, n->targetPos.y);
            maxX = (std::max)(maxX, n->targetPos.x + n->size.width);
            maxY = (std::max)(maxY, n->targetPos.y + n->size.height);
            visibleCount++;
        }
    }

    if (visibleCount == 0) return;

    const float pad = 80.0f;
    const float graphW = maxX - minX + pad * 2.0f;
    const float graphH = maxY - minY + pad * 2.0f;

    const Rect bounds = GetBounds();
    const float viewW = bounds.width > 0 ? bounds.width : 800.0f;
    const float viewH = bounds.height > 0 ? bounds.height : 560.0f;

    float zoomX = viewW / graphW;
    float zoomY = viewH / graphH;
    float targetZoom = (std::clamp)((std::min)(zoomX, zoomY), 0.35f, 1.4f);

    float targetPanX = (viewW - (maxX + minX) * targetZoom) * 0.5f;
    float targetPanY = (viewH - (maxY + minY) * targetZoom) * 0.5f;

    SetZoom(targetZoom, animated);
    SetPanOffset(Point(targetPanX, targetPanY), animated);
}

void TopologyView::CollectDescendants(const std::shared_ptr<TopologyNode>& parent, std::vector<std::shared_ptr<TopologyNode>>& outList) {
    if (!parent) return;
    for (auto& child : parent->children) {
        if (child) {
            outList.push_back(child);
            CollectDescendants(child, outList);
        }
    }
}

void TopologyView::UpdateCollapseVisibilityState() {
    for (auto& n : m_nodes) {
        if (n) {
            n->targetOpacity = 1.0f;
            n->targetScale = 1.0f;
            n->isCulled = false;
        }
    }

    for (auto& n : m_nodes) {
        if (n && n->isCollapsible && !n->isExpanded) {
            std::vector<std::shared_ptr<TopologyNode>> descendants;
            CollectDescendants(n, descendants);
            for (auto& d : descendants) {
                d->targetOpacity = 0.0f;
                d->targetScale = 0.2f;
                d->targetPos = Point(n->targetPos.x + (n->size.width - d->size.width) * 0.5f,
                                     n->targetPos.y + (n->size.height - d->size.height) * 0.5f);
            }
        }
    }

    for (auto& edge : m_edges) {
        auto from = FindNode(edge.fromId);
        auto to = FindNode(edge.toId);
        if (from && to && from->targetOpacity > 0.05f && to->targetOpacity > 0.05f) {
            edge.targetOpacity = 1.0f;
            edge.isCulled = false;
        } else {
            edge.targetOpacity = 0.0f;
        }
    }
}

void TopologyView::Relayout() {
    UpdateCollapseVisibilityState();
    ComputeLayout();
    for (auto& n : m_nodes) {
        if (n && n->currentPos.x == 0.0f && n->currentPos.y == 0.0f) {
            n->currentPos = n->targetPos;
        }
    }
    m_animating = true;
    RequestAnimationTicks();
    MarkRenderContentDirty();
}

void TopologyView::ComputeLayout() {
    switch (m_layoutType) {
    case TopologyLayoutType::HierarchicalLeftRight:
        ComputeHierarchicalLayout(true);
        break;
    case TopologyLayoutType::HierarchicalTopDown:
        ComputeHierarchicalLayout(false);
        break;
    case TopologyLayoutType::Radial:
        ComputeRadialLayout();
        break;
    case TopologyLayoutType::ForceDirected:
        StepForceDirectedSimulation();
        break;
    }
}

// 递归计算子树垂直跨度（Subtree Span），防止跨分支交织重叠
static float GetNodeCrossExtent(const std::shared_ptr<TopologyNode>& node, bool horizontal) {
    if (!node) return 0.0f;
    return horizontal ? node->size.height : node->size.width;
}

// 计算子树在当前布局交叉轴上的完整跨度：水平布局使用高度，垂直布局使用宽度。
static float ComputeNodeSubtreeSpan(const std::shared_ptr<TopologyNode>& node, float siblingGap, bool horizontal) {
    if (!node || node->targetOpacity < 0.05f) return 0.0f;

    const float ownSpan = GetNodeCrossExtent(node, horizontal);
    if (!node->isExpanded || node->children.empty()) return ownSpan;

    float childrenSpan = 0.0f;
    int visibleChildren = 0;
    for (const auto& child : node->children) {
        if (child && child->targetOpacity > 0.05f) {
            if (visibleChildren > 0) childrenSpan += siblingGap;
            childrenSpan += ComputeNodeSubtreeSpan(child, siblingGap, horizontal);
            ++visibleChildren;
        }
    }
    return visibleChildren == 0 ? ownSpan : (std::max)(ownSpan, childrenSpan);
}

static void LayoutSubtree(const std::shared_ptr<TopologyNode>& node, float mainAxis, float crossStart,
                          float levelGap, float siblingGap, bool horizontal) {
    if (!node || node->targetOpacity < 0.05f) return;

    std::vector<std::shared_ptr<TopologyNode>> visibleChildren;
    if (node->isExpanded) {
        for (const auto& child : node->children) {
            if (child && child->targetOpacity > 0.05f) visibleChildren.push_back(child);
        }
    }

    const float subtreeSpan = ComputeNodeSubtreeSpan(node, siblingGap, horizontal);
    if (visibleChildren.empty()) {
        const float crossPosition = crossStart + (subtreeSpan - GetNodeCrossExtent(node, horizontal)) * 0.5f;
        node->targetPos = horizontal ? Point(mainAxis, crossPosition) : Point(crossPosition, mainAxis);
        if (node->currentPos.x == 0.0f && node->currentPos.y == 0.0f) node->currentPos = node->targetPos;
        return;
    }

    float childrenSpan = 0.0f;
    for (size_t index = 0; index < visibleChildren.size(); ++index) {
        if (index > 0) childrenSpan += siblingGap;
        childrenSpan += ComputeNodeSubtreeSpan(visibleChildren[index], siblingGap, horizontal);
    }

    float childCrossStart = crossStart + (subtreeSpan - childrenSpan) * 0.5f;
    float firstChildCenter = 0.0f;
    float lastChildCenter = 0.0f;
    for (size_t index = 0; index < visibleChildren.size(); ++index) {
        const auto& child = visibleChildren[index];
        const float childSpan = ComputeNodeSubtreeSpan(child, siblingGap, horizontal);
        LayoutSubtree(child, mainAxis + levelGap, childCrossStart, levelGap, siblingGap, horizontal);

        const float childCenter = horizontal
            ? child->targetPos.y + child->size.height * 0.5f
            : child->targetPos.x + child->size.width * 0.5f;
        if (index == 0) firstChildCenter = childCenter;
        if (index + 1 == visibleChildren.size()) lastChildCenter = childCenter;
        childCrossStart += childSpan + siblingGap;
    }

    const float childrenCenter = (firstChildCenter + lastChildCenter) * 0.5f;
    node->targetPos = horizontal
        ? Point(mainAxis, childrenCenter - node->size.height * 0.5f)
        : Point(childrenCenter - node->size.width * 0.5f, mainAxis);
    if (node->currentPos.x == 0.0f && node->currentPos.y == 0.0f) node->currentPos = node->targetPos;
}

void TopologyView::ComputeHierarchicalLayout(bool horizontal) {
    if (m_nodes.empty()) return;

    // 1. 查找所有根节点 (无父节点或父节点不在图中的独立顶级节点)
    std::vector<std::shared_ptr<TopologyNode>> rootNodes;
    for (const auto& n : m_nodes) {
        if (n && n->targetOpacity > 0.05f) {
            if (n->parentId.empty() || m_nodeMap.find(n->parentId) == m_nodeMap.end()) {
                rootNodes.push_back(n);
            }
        }
    }

    if (rootNodes.empty() && !m_nodes.empty()) {
        rootNodes.push_back(m_nodes.front());
    }

    const float levelGap = horizontal ? 260.0f : 150.0f;
    const float siblingGap = horizontal ? 24.0f : 30.0f;

    // 2. 依次排布各根节点所在的子树
    float currentTreeY = 40.0f;
    for (const auto& root : rootNodes) {
        float span = ComputeNodeSubtreeSpan(root, siblingGap, horizontal);
        LayoutSubtree(root, 40.0f, currentTreeY, levelGap, siblingGap, horizontal);
        currentTreeY += span + 30.0f;
    }
}

// 递归计算节点的叶子/可见子节点总权重（用于划分角度扇区）
static float ComputeNodeSectorWeight(const std::shared_ptr<TopologyNode>& node) {
    if (!node || node->targetOpacity < 0.05f) return 0.0f;
    if (!node->isExpanded || node->children.empty()) return 1.0f;

    float weight = 0.0f;
    for (const auto& child : node->children) {
        if (child && child->targetOpacity > 0.05f) {
            weight += ComputeNodeSectorWeight(child);
        }
    }
    return (std::max)(1.0f, weight);
}

// 递归扇区分配
static void LayoutRadialSubtree(
    const std::shared_ptr<TopologyNode>& node,
    Point center,
    float startAngle,
    float endAngle,
    float currentRadius,
    float radiusStep)
{
    if (!node || node->targetOpacity < 0.05f) return;

    // 过滤出可见子节点
    std::vector<std::shared_ptr<TopologyNode>> visibleChildren;
    if (node->isExpanded) {
        for (const auto& child : node->children) {
            if (child && child->targetOpacity > 0.05f) {
                visibleChildren.push_back(child);
            }
        }
    }

    if (visibleChildren.empty()) {
        return;
    }

    // 计算总权重
    float totalWeight = 0.0f;
    std::vector<float> childWeights;
    for (const auto& child : visibleChildren) {
        float w = ComputeNodeSectorWeight(child);
        childWeights.push_back(w);
        totalWeight += w;
    }

    if (totalWeight <= 0.0f) totalWeight = 1.0f;

    const float sectorSpan = endAngle - startAngle;
    float curStartAngle = startAngle;

    for (size_t i = 0; i < visibleChildren.size(); ++i) {
        auto child = visibleChildren[i];
        float childSector = (childWeights[i] / totalWeight) * sectorSpan;
        float childMidAngle = curStartAngle + childSector * 0.5f;

        // 计算子节点在同心圆环上的坐标
        float nextRadius = currentRadius + radiusStep;
        child->targetPos = Point(
            center.x + std::cos(childMidAngle) * nextRadius - child->size.width * 0.5f,
            center.y + std::sin(childMidAngle) * nextRadius - child->size.height * 0.5f
        );

        if (child->currentPos.x == 0.0f && child->currentPos.y == 0.0f) {
            child->currentPos = child->targetPos;
        }

        // 递归为下一级分配子扇区
        LayoutRadialSubtree(child, center, curStartAngle, curStartAngle + childSector, nextRadius, radiusStep);

        curStartAngle += childSector;
    }
}

void TopologyView::ComputeRadialLayout() {
    if (m_nodes.empty()) return;

    // 1. 查找根节点
    std::vector<std::shared_ptr<TopologyNode>> rootNodes;
    for (const auto& n : m_nodes) {
        if (n && n->targetOpacity > 0.05f) {
            if (n->parentId.empty() || m_nodeMap.find(n->parentId) == m_nodeMap.end()) {
                rootNodes.push_back(n);
            }
        }
    }

    if (rootNodes.empty() && !m_nodes.empty()) {
        rootNodes.push_back(m_nodes.front());
    }

    const Point center(500.0f, 360.0f);
    const float radiusStep = 230.0f;

    if (rootNodes.size() == 1) {
        auto root = rootNodes.front();
        root->targetPos = Point(center.x - root->size.width * 0.5f, center.y - root->size.height * 0.5f);
        if (root->currentPos.x == 0.0f && root->currentPos.y == 0.0f) {
            root->currentPos = root->targetPos;
        }
        LayoutRadialSubtree(root, center, -static_cast<float>(M_PI), static_cast<float>(M_PI), 0.0f, radiusStep);
    } else {
        float totalRootWeight = 0.0f;
        std::vector<float> rootWeights;
        for (const auto& r : rootNodes) {
            float w = ComputeNodeSectorWeight(r);
            rootWeights.push_back(w);
            totalRootWeight += w;
        }
        if (totalRootWeight <= 0.0f) totalRootWeight = 1.0f;

        float curAngle = -static_cast<float>(M_PI);
        for (size_t i = 0; i < rootNodes.size(); ++i) {
            auto root = rootNodes[i];
            float span = (rootWeights[i] / totalRootWeight) * (2.0f * static_cast<float>(M_PI));
            float midAngle = curAngle + span * 0.5f;

            root->targetPos = Point(
                center.x + std::cos(midAngle) * 90.0f - root->size.width * 0.5f,
                center.y + std::sin(midAngle) * 90.0f - root->size.height * 0.5f
            );
            if (root->currentPos.x == 0.0f && root->currentPos.y == 0.0f) {
                root->currentPos = root->targetPos;
            }

            LayoutRadialSubtree(root, center, curAngle, curAngle + span, 90.0f, radiusStep);
            curAngle += span;
        }
    }
}

void TopologyView::StepForceDirectedSimulation() {
    const float kRepulsion = 14000.0f;
    const float kSpring = 0.05f;
    const float springLength = 220.0f;
    const float damping = 0.82f;

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        auto a = m_nodes[i];
        if (!a || a->targetOpacity < 0.05f) continue;

        for (size_t j = i + 1; j < m_nodes.size(); ++j) {
            auto b = m_nodes[j];
            if (!b || b->targetOpacity < 0.05f) continue;

            float dx = b->targetPos.x - a->targetPos.x;
            float dy = b->targetPos.y - a->targetPos.y;
            float dist = (std::max)(20.0f, std::sqrt(dx * dx + dy * dy));
            float force = kRepulsion / (dist * dist);

            float fx = (dx / dist) * force;
            float fy = (dy / dist) * force;

            a->velocity.x -= fx;
            a->velocity.y -= fy;
            b->velocity.x += fx;
            b->velocity.y += fy;
        }
    }

    for (const auto& edge : m_edges) {
        auto a = FindNode(edge.fromId);
        auto b = FindNode(edge.toId);
        if (!a || !b || a->targetOpacity < 0.05f || b->targetOpacity < 0.05f) continue;

        float dx = b->targetPos.x - a->targetPos.x;
        float dy = b->targetPos.y - a->targetPos.y;
        float dist = (std::max)(1.0f, std::sqrt(dx * dx + dy * dy));
        float force = (dist - springLength) * kSpring;

        float fx = (dx / dist) * force;
        float fy = (dy / dist) * force;

        a->velocity.x += fx;
        a->velocity.y += fy;
        b->velocity.x += fx;
        b->velocity.y += fy;
    }

    for (auto& n : m_nodes) {
        if (!n || n->targetOpacity < 0.05f || n->isDragging) continue;
        n->velocity.x *= damping;
        n->velocity.y *= damping;
        n->targetPos.x += n->velocity.x;
        n->targetPos.y += n->velocity.y;
    }
}

bool TopologyView::HasSelfAnimation() const {
    return m_animating || m_flowParticlesEnabled;
}

bool TopologyView::OnAnimationTick() {
    bool childAnimating = UIElement::OnAnimationTick();
    const float dt = (std::clamp)(UIElement::GetAnimationDeltaSeconds(), 0.0f, 0.05f);
    bool keepAnimating = false;
    bool visualChanged = false;

    if (m_zoomAnimator.IsActive()) {
        m_zoomAnimator.Tick(dt, 0.2f, 3.5f);
        m_zoom = m_zoomAnimator.Current();
        keepAnimating = true;
        visualChanged = true;
    }
    if (m_panXAnimator.IsActive()) {
        m_panXAnimator.Tick(dt, -50000.0f, 50000.0f);
        m_panOffset.x = m_panXAnimator.Current();
        keepAnimating = true;
        visualChanged = true;
    }
    if (m_panYAnimator.IsActive()) {
        m_panYAnimator.Tick(dt, -50000.0f, 50000.0f);
        m_panOffset.y = m_panYAnimator.Current();
        keepAnimating = true;
        visualChanged = true;
    }

    // 固定时间常数，而非每帧乘固定比例，动画速度不再随帧率变化。
    const float positionAlpha = 1.0f - std::exp(-dt / 0.055f);
    const float appearanceAlpha = 1.0f - std::exp(-dt / 0.040f);
    for (auto& node : m_nodes) {
        if (!node) continue;
        const float dx = node->targetPos.x - node->currentPos.x;
        const float dy = node->targetPos.y - node->currentPos.y;
        const float da = node->targetOpacity - node->opacity;
        const float ds = node->targetScale - node->scale;
        if (std::abs(dx) > 0.15f || std::abs(dy) > 0.15f) {
            node->currentPos.x += dx * positionAlpha;
            node->currentPos.y += dy * positionAlpha;
            keepAnimating = true;
            visualChanged = true;
        } else node->currentPos = node->targetPos;
        if (std::abs(da) > 0.005f) {
            node->opacity += da * appearanceAlpha;
            keepAnimating = true;
            visualChanged = true;
        } else node->opacity = node->targetOpacity;
        if (std::abs(ds) > 0.005f) {
            node->scale += ds * appearanceAlpha;
            keepAnimating = true;
            visualChanged = true;
        } else node->scale = node->targetScale;
        node->isCulled = node->opacity <= 0.03f && node->targetOpacity <= 0.0f;
    }

    for (auto& edge : m_edges) {
        const float da = edge.targetOpacity - edge.opacity;
        if (std::abs(da) > 0.005f) {
            edge.opacity += da * appearanceAlpha;
            keepAnimating = true;
            visualChanged = true;
        } else edge.opacity = edge.targetOpacity;
        edge.isCulled = edge.opacity <= 0.03f && edge.targetOpacity <= 0.0f;
    }

    // 粒子按 30 FPS 更新；其它动画仍可按窗口时钟运行，但不再每帧强制重绘粒子。
    if (m_flowParticlesEnabled) {
        constexpr float particleInterval = 1.0f / 30.0f;
        m_particleFrameAccumulator += dt;
        if (m_particleFrameAccumulator >= particleInterval) {
            const float particleDt = (std::min)(m_particleFrameAccumulator, particleInterval * 2.0f);
            m_particleFrameAccumulator = std::fmod(m_particleFrameAccumulator, particleInterval);
            for (auto& edge : m_edges) {
                if (edge.isFlowActive && !edge.isCulled) {
                    edge.flowPhase = std::fmod(edge.flowPhase + particleDt * 0.72f * edge.flowSpeed, 1.0f);
                }
            }
            m_particleTick += particleDt * 1.2f;
            visualChanged = true;
        }
    } else {
        m_particleFrameAccumulator = 0.0f;
    }

    m_animating = keepAnimating;
    if (visualChanged) MarkRenderRectDirty(GetBounds());
    return childAnimating || keepAnimating || m_flowParticlesEnabled;
}

Point TopologyView::WorldToScreen(Point worldPt) const {
    const Rect bounds = GetBounds();
    return Point(bounds.x + m_panOffset.x + worldPt.x * m_zoom,
                 bounds.y + m_panOffset.y + worldPt.y * m_zoom);
}

Point TopologyView::ScreenToWorld(Point screenPt) const {
    const Rect bounds = GetBounds();
    return Point((screenPt.x - bounds.x - m_panOffset.x) / m_zoom,
                 (screenPt.y - bounds.y - m_panOffset.y) / m_zoom);
}

Rect TopologyView::GetNodeBounds(const std::shared_ptr<TopologyNode>& node) const {
    if (!node) return Rect(0, 0, 0, 0);
    Point screenPos = WorldToScreen(node->currentPos);
    return Rect(screenPos.x, screenPos.y, node->size.width * m_zoom * node->scale, node->size.height * m_zoom * node->scale);
}

Rect TopologyView::GetCollapseButtonBounds(const std::shared_ptr<TopologyNode>& node) const {
    Rect cardRect = GetNodeBounds(node);
    const float btnSize = 20.0f * m_zoom;
    return Rect(cardRect.x + cardRect.width - btnSize - 6.0f * m_zoom,
                cardRect.y + cardRect.height - btnSize - 6.0f * m_zoom,
                btnSize, btnSize);
}

std::shared_ptr<TopologyNode> TopologyView::HitTestNode(Point screenPt) {
    for (auto it = m_nodes.rbegin(); it != m_nodes.rend(); ++it) {
        auto node = *it;
        if (!node || node->isCulled || node->opacity < 0.1f) continue;
        Rect r = GetNodeBounds(node);
        if (r.Contains(screenPt.x, screenPt.y)) {
            return node;
        }
    }
    return nullptr;
}

std::shared_ptr<TopologyNode> TopologyView::HitTestCollapseButton(Point screenPt) {
    for (auto it = m_nodes.rbegin(); it != m_nodes.rend(); ++it) {
        auto node = *it;
        if (!node || node->isCulled || !node->isCollapsible || node->opacity < 0.1f) continue;
        Rect r = GetCollapseButtonBounds(node);
        if (r.Contains(screenPt.x, screenPt.y)) {
            return node;
        }
    }
    return nullptr;
}

void TopologyView::UpdateHoverHighlights(const std::shared_ptr<TopologyNode>& hovered) {
    if (!hovered) {
        for (auto& n : m_nodes) { if (n) { n->isHighlighted = false; n->isDimmed = false; } }
        for (auto& e : m_edges) { e.isHighlighted = false; e.isDimmed = false; }
        return;
    }

    std::unordered_set<std::string> connectedNodes;
    connectedNodes.insert(hovered->id);

    for (auto& e : m_edges) {
        if (e.fromId == hovered->id || e.toId == hovered->id) {
            e.isHighlighted = true;
            e.isDimmed = false;
            connectedNodes.insert(e.fromId);
            connectedNodes.insert(e.toId);
        } else {
            e.isHighlighted = false;
            e.isDimmed = true;
        }
    }

    for (auto& n : m_nodes) {
        if (!n) continue;
        if (connectedNodes.find(n->id) != connectedNodes.end()) {
            n->isHighlighted = true;
            n->isDimmed = false;
        } else {
            n->isHighlighted = false;
            n->isDimmed = true;
        }
    }
}

void TopologyView::OnMouseDown(Point pt) {
    auto collapseBtnNode = HitTestCollapseButton(pt);
    if (collapseBtnNode) {
        ToggleNode(collapseBtnNode->id);
        return;
    }

    auto hitNode = HitTestNode(pt);
    if (hitNode) {
        SetSelectedItem(hitNode);
        if (!m_isReadOnly) {
            m_draggingNode = hitNode;
            m_draggingNode->isDragging = true;
        }
        m_lastMousePos = pt;
        m_onNodeClicked.Invoke(this, hitNode);
    } else {
        SetSelectedItem(nullptr);
        m_isPanning = true;
        m_lastMousePos = pt;
    }
    MarkRenderRectDirty(GetBounds());
}

void TopologyView::OnMouseMove(Point pt) {
    if (m_isPanning) {
        float dx = (pt.x - m_lastMousePos.x);
        float dy = (pt.y - m_lastMousePos.y);
        m_panOffset.x += dx;
        m_panOffset.y += dy;
        m_panXAnimator.JumpTo(m_panOffset.x);
        m_panYAnimator.JumpTo(m_panOffset.y);
        m_lastMousePos = pt;
        MarkRenderRectDirty(GetBounds());
        return;
    }

    if (!m_isReadOnly && m_draggingNode) {
        Point deltaWorld((pt.x - m_lastMousePos.x) / m_zoom, (pt.y - m_lastMousePos.y) / m_zoom);
        m_draggingNode->targetPos.x += deltaWorld.x;
        m_draggingNode->targetPos.y += deltaWorld.y;
        m_draggingNode->currentPos = m_draggingNode->targetPos;
        m_lastMousePos = pt;
        MarkRenderRectDirty(GetBounds());
        return;
    }

    auto hitNode = HitTestNode(pt);
    if (hitNode != m_hoveredNode) {
        if (m_hoveredNode) m_hoveredNode->isHovered = false;
        m_hoveredNode = hitNode;
        if (m_hoveredNode) m_hoveredNode->isHovered = true;
        UpdateHoverHighlights(m_hoveredNode);
        MarkRenderRectDirty(GetBounds());
    }
}

void TopologyView::OnMouseUp(Point pt) {
    (void)pt;
    if (m_draggingNode) {
        m_draggingNode->isDragging = false;
        m_draggingNode = nullptr;
    }
    m_isPanning = false;
    MarkRenderContentDirty();
}

void TopologyView::OnMouseLeave() {
    if (m_hoveredNode) {
        m_hoveredNode->isHovered = false;
        m_hoveredNode = nullptr;
        UpdateHoverHighlights(nullptr);
        MarkRenderContentDirty();
    }
    m_isPanning = false;
    if (m_draggingNode) {
        m_draggingNode->isDragging = false;
        m_draggingNode = nullptr;
    }
}

void TopologyView::OnMouseDblClick(Point pt) {
    auto hitNode = HitTestNode(pt);
    if (hitNode) {
        m_onNodeDoubleClicked.Invoke(this, hitNode);
        if (hitNode->isCollapsible) {
            ToggleNode(hitNode->id);
        }
    }
}

void TopologyView::OnMouseWheel(float delta) {
    const Rect bounds = GetBounds();
    const Point center(bounds.width * 0.5f, bounds.height * 0.5f);
    const float factor = (delta > 0) ? 1.18f : 0.85f;
    const float curTarget = m_zoomAnimator.Target();
    const float newZoom = (std::clamp)(curTarget * factor, 0.25f, 3.0f);

    if (std::abs(newZoom - curTarget) > 0.001f) {
        float curPanX = m_panXAnimator.Target();
        float curPanY = m_panYAnimator.Target();
        float targetPanX = center.x - (center.x - curPanX) * (newZoom / curTarget);
        float targetPanY = center.y - (center.y - curPanY) * (newZoom / curTarget);

        SetZoom(newZoom, true);
        SetPanOffset(Point(targetPanX, targetPanY), true);
    }
}

bool TopologyView::OnKeyDown(int vkCode) {
    if (vkCode == VK_SPACE || vkCode == 'F') {
        FitToView();
        return true;
    }
    if (vkCode == 'R') {
        ResetView();
        return true;
    }
    if (!m_isReadOnly && (vkCode == VK_DELETE || vkCode == VK_BACK)) {
        if (m_selectedNode) {
            RemoveNode(m_selectedNode->id);
            return true;
        }
    }
    return false;
}

static inline Point EvalCubicBezier(Point p0, Point cp0, Point cp1, Point p1, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    return Point(
        uuu * p0.x + 3.0f * uu * t * cp0.x + 3.0f * u * tt * cp1.x + ttt * p1.x,
        uuu * p0.y + 3.0f * uu * t * cp0.y + 3.0f * u * tt * cp1.y + ttt * p1.y
    );
}

static void DrawFastBezier(GraphicsContext& ctx, Point p1, Point cp1, Point cp2, Point p2, D2D1_COLOR_F color, float strokeWidth) {
    constexpr int kSegments = 6;
    Point prev = p1;
    for (int i = 1; i <= kSegments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSegments);
        Point curr = EvalCubicBezier(p1, cp1, cp2, p2, t);
        ctx.DrawSmoothLine(prev, curr, color, strokeWidth);
        prev = curr;
    }
}

static inline void FillFastCircle(GraphicsContext& ctx, Point center, float radius, D2D1_COLOR_F color) {
    ctx.FillRoundedRect(Rect(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f), radius, color);
}

void TopologyView::OnRender(GraphicsContext& ctx) {
    const Rect bounds = GetBounds();
    ctx.PushClip(bounds);

    RenderGridBackground(ctx);
    RenderEdges(ctx);
    RenderNodes(ctx);
    RenderMinimapAndToolbar(ctx);

    ctx.PopClip();
}

void TopologyView::RenderGridBackground(GraphicsContext& ctx) {
    const Rect bounds = GetBounds();
    const bool isDark = (ThemeManager::Instance().GetThemeMode() == ThemeMode::Dark);
    const D2D1_COLOR_F bgColor = isDark ? D2D1::ColorF(0x141416, 1.0f) : D2D1::ColorF(0xF8F9FA, 1.0f);
    const D2D1_COLOR_F gridLineColor = isDark ? D2D1::ColorF(0x202024, 0.7f) : D2D1::ColorF(0xEBEEF2, 0.9f);

    ctx.FillRect(bounds, bgColor);

    const float gridSize = 36.0f * m_zoom;
    if (gridSize < 16.0f) return;

    const float startX = std::fmod(m_panOffset.x, gridSize);
    const float startY = std::fmod(m_panOffset.y, gridSize);

    for (float x = bounds.x + startX; x < bounds.x + bounds.width; x += gridSize) {
        ctx.DrawLine(Point(x, bounds.y), Point(x, bounds.y + bounds.height), gridLineColor, 1.0f);
    }
    for (float y = bounds.y + startY; y < bounds.y + bounds.height; y += gridSize) {
        ctx.DrawLine(Point(bounds.x, y), Point(bounds.x + bounds.width, y), gridLineColor, 1.0f);
    }
}

static D2D1_COLOR_F GetStatusColor(TopologyNodeStatus status, bool isDark) {
    switch (status) {
    case TopologyNodeStatus::Success: return isDark ? D2D1::ColorF(0x10B981, 1.0f) : D2D1::ColorF(0x059669, 1.0f);
    case TopologyNodeStatus::Warning: return isDark ? D2D1::ColorF(0xF59E0B, 1.0f) : D2D1::ColorF(0xD97706, 1.0f);
    case TopologyNodeStatus::Error: return isDark ? D2D1::ColorF(0xEF4444, 1.0f) : D2D1::ColorF(0xDC2626, 1.0f);
    case TopologyNodeStatus::Inactive: return isDark ? D2D1::ColorF(0x71717A, 0.7f) : D2D1::ColorF(0x94A3B8, 0.8f);
    case TopologyNodeStatus::Processing: return isDark ? D2D1::ColorF(0x8B5CF6, 1.0f) : D2D1::ColorF(0x7C3AED, 1.0f);
    case TopologyNodeStatus::Normal:
    default:
        return isDark ? D2D1::ColorF(0x3B82F6, 1.0f) : D2D1::ColorF(0x2563EB, 1.0f);
    }
}

void TopologyView::RenderEdges(GraphicsContext& ctx) {
    const bool isDark = (ThemeManager::Instance().GetThemeMode() == ThemeMode::Dark);
    int particleBudget = m_zoom >= 0.85f ? 96 : 48;

    for (const auto& edge : m_edges) {
        if (edge.isCulled || edge.opacity <= 0.01f) continue;

        auto from = FindNode(edge.fromId);
        auto to = FindNode(edge.toId);
        if (!from || !to || from->isCulled || to->isCulled) continue;

        const Rect rFrom = GetNodeBounds(from);
        const Rect rTo = GetNodeBounds(to);

        Point p1(rFrom.x + rFrom.width, rFrom.y + rFrom.height * 0.5f);
        Point p2(rTo.x, rTo.y + rTo.height * 0.5f);
        Point cp1, cp2;

        if (m_layoutType == TopologyLayoutType::Radial || m_layoutType == TopologyLayoutType::ForceDirected) {
            p1 = Point(rFrom.x + rFrom.width * 0.5f, rFrom.y + rFrom.height * 0.5f);
            p2 = Point(rTo.x + rTo.width * 0.5f, rTo.y + rTo.height * 0.5f);
            cp1 = Point(p1.x + (p2.x - p1.x) * 0.33f, p1.y + (p2.y - p1.y) * 0.33f);
            cp2 = Point(p1.x + (p2.x - p1.x) * 0.66f, p1.y + (p2.y - p1.y) * 0.66f);
        } else if (m_layoutType == TopologyLayoutType::HierarchicalTopDown) {
            p1 = Point(rFrom.x + rFrom.width * 0.5f, rFrom.y + rFrom.height);
            p2 = Point(rTo.x + rTo.width * 0.5f, rTo.y);
            const float dy = p2.y - p1.y;
            cp1 = Point(p1.x, p1.y + (std::max)(30.0f, dy * 0.45f));
            cp2 = Point(p2.x, p2.y - (std::max)(30.0f, dy * 0.45f));
        } else {
            const float dx = p2.x - p1.x;
            cp1 = Point(p1.x + (std::max)(40.0f, dx * 0.45f), p1.y);
            cp2 = Point(p2.x - (std::max)(40.0f, dx * 0.45f), p2.y);
        }

        D2D1_COLOR_F strokeColor = edge.customColor.value_or(
            edge.isHighlighted ? (isDark ? D2D1::ColorF(0x60A5FA, 1.0f) : D2D1::ColorF(0x2563EB, 1.0f)) :
            (isDark ? D2D1::ColorF(0x4B5563, 0.75f) : D2D1::ColorF(0x94A3B8, 0.8f))
        );

        if (edge.isDimmed) strokeColor.a *= 0.25f;
        strokeColor.a *= edge.opacity;

        const float strokeThickness = (edge.isHighlighted ? 2.5f : 1.5f) * m_zoom;

        DrawFastBezier(ctx, p1, cp1, cp2, p2, strokeColor, strokeThickness);

        if (m_flowParticlesEnabled && edge.isFlowActive && edge.opacity > 0.3f && !edge.isDimmed && m_zoom >= 0.45f) {
            const float edgeLeft = (std::min)((std::min)(p1.x, p2.x), (std::min)(cp1.x, cp2.x));
            const float edgeTop = (std::min)((std::min)(p1.y, p2.y), (std::min)(cp1.y, cp2.y));
            const float edgeRight = (std::max)((std::max)(p1.x, p2.x), (std::max)(cp1.x, cp2.x));
            const float edgeBottom = (std::max)((std::max)(p1.y, p2.y), (std::max)(cp1.y, cp2.y));
            const Rect edgeBounds(edgeLeft, edgeTop, edgeRight - edgeLeft, edgeBottom - edgeTop);
            const int particleCount = m_zoom >= 0.85f ? 2 : 1;
            if (edgeBounds.Inflate(8.0f * m_zoom).Intersects(GetBounds()) && particleBudget >= particleCount) {
                for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
                    const float phaseOffset = particleCount == 2 ? static_cast<float>(particleIndex) * 0.5f : 0.0f;
                    const float t = std::fmod(edge.flowPhase + phaseOffset, 1.0f);
                    const Point particlePos = EvalCubicBezier(p1, cp1, cp2, p2, t);
                    D2D1_COLOR_F glowColor = strokeColor;
                    glowColor.a = 0.9f * edge.opacity;
                    FillFastCircle(ctx, particlePos, 3.2f * m_zoom, glowColor);
                }
                particleBudget -= particleCount;
            }
        }

        if (!edge.label.empty() && edge.opacity > 0.4f && m_zoom > 0.6f) {
            Point mid(
                0.125f * p1.x + 0.375f * cp1.x + 0.375f * cp2.x + 0.125f * p2.x,
                0.125f * p1.y + 0.375f * cp1.y + 0.375f * cp2.y + 0.125f * p2.y
            );
            const float labelW = 56.0f * m_zoom;
            const float labelH = 18.0f * m_zoom;
            Rect labelRect(mid.x - labelW * 0.5f, mid.y - labelH * 0.5f, labelW, labelH);

            D2D1_COLOR_F pillBg = isDark ? D2D1::ColorF(0x27272A, 0.92f) : D2D1::ColorF(0xFFFFFF, 0.95f);
            ctx.FillRoundedRect(labelRect, 4.0f * m_zoom, pillBg);
            ctx.DrawRoundedRect(labelRect, 4.0f * m_zoom, strokeColor, 1.0f);

            D2D1_COLOR_F labelTextColor = isDark ? D2D1::ColorF(0xE4E4E7, 0.9f) : D2D1::ColorF(0x334155, 0.9f);
            ctx.DrawText(edge.label, labelRect, labelTextColor, "Segoe UI", 10.0f * m_zoom,
                         DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

void TopologyView::RenderNodes(GraphicsContext& ctx) {
    const bool isDark = (ThemeManager::Instance().GetThemeMode() == ThemeMode::Dark);

    for (const auto& node : m_nodes) {
        if (!node || node->isCulled || node->opacity <= 0.01f) continue;

        const Rect cardRect = GetNodeBounds(node);
        const float radius = 8.0f * m_zoom * node->scale;

        D2D1_COLOR_F cardBg = isDark ? D2D1::ColorF(0x222226, 0.94f) : D2D1::ColorF(0xFFFFFF, 0.96f);
        if (node->isHighlighted) {
            cardBg = isDark ? D2D1::ColorF(0x2D3748, 0.95f) : D2D1::ColorF(0xF0F7FF, 0.98f);
        }
        cardBg.a *= node->opacity;
        if (node->isDimmed) cardBg.a *= 0.35f;

        // 选中高亮光晕
        if (node->isSelected && node->opacity > 0.5f) {
            Rect glowRect(cardRect.x - 3.0f * m_zoom, cardRect.y - 3.0f * m_zoom,
                          cardRect.width + 6.0f * m_zoom, cardRect.height + 6.0f * m_zoom);
            D2D1_COLOR_F glowColor = D2D1::ColorF(0x3B82F6, 0.45f * node->opacity);
            ctx.DrawRoundedRect(glowRect, radius + 2.0f, glowColor, 3.0f * m_zoom);
        }

        ctx.FillRoundedRect(cardRect, radius, cardBg);

        D2D1_COLOR_F borderColor = node->customColor.value_or(
            node->isSelected ? D2D1::ColorF(0x3B82F6, 1.0f) :
            (node->isHovered ? (isDark ? D2D1::ColorF(0x60A5FA, 0.85f) : D2D1::ColorF(0x3B82F6, 0.85f)) :
            (isDark ? D2D1::ColorF(0x3F3F46, 0.8f) : D2D1::ColorF(0xCBD5E1, 0.85f)))
        );
        borderColor.a *= node->opacity;
        if (node->isDimmed) borderColor.a *= 0.3f;

        ctx.DrawRoundedRect(cardRect, radius, borderColor, (node->isSelected ? 2.0f : 1.0f) * m_zoom);

        // 状态指示灯
        const D2D1_COLOR_F statusColor = GetStatusColor(node->status, isDark);
        Point statusDotPos(cardRect.x + 14.0f * m_zoom * node->scale, cardRect.y + 16.0f * m_zoom * node->scale);
        
        if (node->status == TopologyNodeStatus::Error || node->status == TopologyNodeStatus::Processing) {
            float pulseScale = 1.0f + 0.3f * std::sin(m_particleTick * 3.0f);
            D2D1_COLOR_F haloColor = statusColor;
            haloColor.a = 0.35f * node->opacity;
            FillFastCircle(ctx, statusDotPos, 6.0f * m_zoom * pulseScale, haloColor);
        }
        FillFastCircle(ctx, statusDotPos, 4.0f * m_zoom, statusColor);

        // 图标
        float textLeft = cardRect.x + 26.0f * m_zoom * node->scale;
        if (!node->icon.empty()) {
            Rect iconRect(textLeft, cardRect.y + 8.0f * m_zoom * node->scale, 20.0f * m_zoom, 20.0f * m_zoom);
            D2D1_COLOR_F iconColor = isDark ? D2D1::ColorF(0xFAFAFA, node->opacity) : D2D1::ColorF(0x0F172A, node->opacity);
            if (node->isDimmed) iconColor.a *= 0.35f;
            ctx.DrawText(node->icon, iconRect, iconColor, "Segoe UI Emoji", 13.0f * m_zoom * node->scale,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            textLeft += 22.0f * m_zoom * node->scale;
        }

        // 计算右上角 Badge 尺寸，避免与标题文本交织重叠
        float badgeW = 0.0f;
        if (!node->badge.empty() && m_zoom > 0.5f) {
            badgeW = (std::clamp)(static_cast<float>(node->badge.size()) * 7.5f + 14.0f, 48.0f, 85.0f) * m_zoom * node->scale;
            Rect badgeRect(cardRect.x + cardRect.width - badgeW - 8.0f * m_zoom, cardRect.y + 8.0f * m_zoom, badgeW, 18.0f * m_zoom * node->scale);

            D2D1_COLOR_F badgeBg = isDark ? D2D1::ColorF(0x3F3F46, 0.65f * node->opacity) : D2D1::ColorF(0xE2E8F0, 0.85f * node->opacity);
            ctx.FillRoundedRect(badgeRect, 3.0f * m_zoom, badgeBg);

            D2D1_COLOR_F badgeTextColor = isDark ? D2D1::ColorF(0xD4D4D8, node->opacity) : D2D1::ColorF(0x475569, node->opacity);
            ctx.DrawText(node->badge, badgeRect, badgeTextColor, "Segoe UI", 9.5f * m_zoom * node->scale,
                         DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // 标题文本（明确限制宽度，开启省略号截断，杜绝文字重叠）
        const float titleAvailableW = (std::max)(20.0f, cardRect.width - (textLeft - cardRect.x) - (badgeW > 0 ? (badgeW + 14.0f * m_zoom) : 10.0f * m_zoom));
        Rect titleRect(textLeft, cardRect.y + 7.0f * m_zoom * node->scale, titleAvailableW, 20.0f * m_zoom * node->scale);
        D2D1_COLOR_F titleColor = isDark ? D2D1::ColorF(0xF4F4F5, node->opacity) : D2D1::ColorF(0x0F172A, node->opacity);
        if (node->isDimmed) titleColor.a *= 0.35f;
        ctx.DrawText(node->label, titleRect, titleColor, "Segoe UI", 11.5f * m_zoom * node->scale,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD, true);

        // 副标题 / 类别
        if (!node->subLabel.empty() || !node->category.empty()) {
            std::string sub = !node->subLabel.empty() ? node->subLabel : node->category;
            const float subAvailableW = (std::max)(20.0f, cardRect.width - 28.0f * m_zoom - (node->isCollapsible ? 26.0f * m_zoom : 0.0f));
            Rect subRect(cardRect.x + 14.0f * m_zoom * node->scale, cardRect.y + 34.0f * m_zoom * node->scale,
                         subAvailableW, 16.0f * m_zoom * node->scale);
            D2D1_COLOR_F subColor = isDark ? D2D1::ColorF(0xA1A1AA, 0.9f * node->opacity) : D2D1::ColorF(0x64748B, 0.9f * node->opacity);
            if (node->isDimmed) subColor.a *= 0.35f;
            ctx.DrawText(sub, subRect, subColor, "Segoe UI", 10.0f * m_zoom * node->scale,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL, true);
        }

        // 折叠切换按钮 (+ / -)
        if (node->isCollapsible && m_zoom > 0.5f) {
            Rect btnRect = GetCollapseButtonBounds(node);
            D2D1_COLOR_F btnBg = isDark ? D2D1::ColorF(0x3F3F46, 0.8f * node->opacity) : D2D1::ColorF(0xE2E8F0, 0.95f * node->opacity);
            if (btnRect.Contains(m_lastMousePos.x, m_lastMousePos.y)) {
                btnBg = isDark ? D2D1::ColorF(0x3B82F6, 0.9f * node->opacity) : D2D1::ColorF(0x2563EB, 0.9f * node->opacity);
            }
            ctx.FillRoundedRect(btnRect, 4.0f * m_zoom, btnBg);

            std::string btnSymbol = node->isExpanded ? "-" : "+";
            D2D1_COLOR_F symbolColor = isDark ? D2D1::ColorF(0xFFFFFF, node->opacity) : D2D1::ColorF(0x1E293B, node->opacity);
            ctx.DrawText(btnSymbol, btnRect, symbolColor, "Segoe UI", 12.0f * m_zoom * node->scale,
                         DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        }
    }
}

void TopologyView::RenderMinimapAndToolbar(GraphicsContext& ctx) {
    const Rect bounds = GetBounds();
    const bool isDark = (ThemeManager::Instance().GetThemeMode() == ThemeMode::Dark);

    const float hudW = 210.0f;
    const float hudH = 34.0f;
    Rect hudRect(bounds.x + bounds.width - hudW - 14.0f, bounds.y + bounds.height - hudH - 14.0f, hudW, hudH);

    D2D1_COLOR_F hudBg = isDark ? D2D1::ColorF(0x1F1F23, 0.92f) : D2D1::ColorF(0xFFFFFF, 0.95f);
    D2D1_COLOR_F hudBorder = isDark ? D2D1::ColorF(0x3F3F46, 0.7f) : D2D1::ColorF(0xCBD5E1, 0.8f);

    ctx.FillRoundedRect(hudRect, 6.0f, hudBg);
    ctx.DrawRoundedRect(hudRect, 6.0f, hudBorder, 1.0f);

    std::string modeText = m_isReadOnly ? "只读" : "可编辑";
    std::string zoomText = std::format("{:.0f}%", m_zoom * 100.0f);
    D2D1_COLOR_F textColor = isDark ? D2D1::ColorF(0xD4D4D8, 1.0f) : D2D1::ColorF(0x334155, 1.0f);

    ctx.DrawText(std::format("[{}] 缩放: {} | [F] 适配", modeText, zoomText), hudRect, textColor, "Segoe UI", 10.5f,
                 DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

} // namespace CUI
