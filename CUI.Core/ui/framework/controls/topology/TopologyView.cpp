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
{
    SetWidth(800.0f);
    SetHeight(500.0f);
}

HCURSOR TopologyView::GetCursor() const {
    if (m_isPanning) return LoadCursor(nullptr, IDC_SIZEALL);
    if (m_draggingNode) return LoadCursor(nullptr, IDC_HAND);
    if (m_hoveredNode) return LoadCursor(nullptr, IDC_HAND);
    return LoadCursor(nullptr, IDC_ARROW);
}

Size TopologyView::Measure(Size availableSize) {
    float w = (GetWidth() > 0.0f) ? GetWidth() : ((availableSize.width > 0.0f && availableSize.width < 10000.0f) ? availableSize.width : 800.0f);
    float h = (GetHeight() > 0.0f) ? GetHeight() : ((availableSize.height > 0.0f && availableSize.height < 10000.0f) ? availableSize.height : 500.0f);
    return Size(w, h);
}

void TopologyView::AddNode(std::shared_ptr<TopologyNode> node) {
    if (!node || node->id.empty()) return;
    if (m_nodeMap.find(node->id) == m_nodeMap.end()) {
        m_nodes.push_back(node);
        m_nodeMap[node->id] = node;
        Relayout();
    }
}

void TopologyView::AddEdge(TopologyEdge edge) {
    m_edges.push_back(edge);
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
    MarkRenderContentDirty();
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

void TopologyView::SetZoom(float zoom) {
    m_zoom = (std::clamp)(zoom, 0.2f, 3.5f);
    MarkRenderContentDirty();
}

void TopologyView::SetPanOffset(Point offset) {
    m_panOffset = offset;
    MarkRenderContentDirty();
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

void TopologyView::ResetView() {
    m_zoom = 1.0f;
    m_panOffset = Point(60.0f, 60.0f);
    Relayout();
}

void TopologyView::FitToView() {
    if (m_nodes.empty()) return;

    float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
    int visibleCount = 0;
    for (const auto& n : m_nodes) {
        if (n && !n->isCulled) {
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

    const float viewW = GetBounds().width > 0 ? GetBounds().width : 800.0f;
    const float viewH = GetBounds().height > 0 ? GetBounds().height : 500.0f;

    float zoomX = viewW / graphW;
    float zoomY = viewH / graphH;
    m_zoom = (std::clamp)((std::min)(zoomX, zoomY), 0.35f, 1.6f);

    m_panOffset.x = (viewW - (maxX + minX) * m_zoom) * 0.5f;
    m_panOffset.y = (viewH - (maxY + minY) * m_zoom) * 0.5f;

    MarkRenderContentDirty();
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

void TopologyView::ComputeHierarchicalLayout(bool horizontal) {
    if (m_nodes.empty()) return;

    std::unordered_map<std::string, int> inDegree;
    for (const auto& n : m_nodes) {
        if (n && !n->isCulled && n->targetOpacity > 0.05f) {
            inDegree[n->id] = 0;
        }
    }

    for (const auto& edge : m_edges) {
        if (inDegree.find(edge.toId) != inDegree.end() && inDegree.find(edge.fromId) != inDegree.end()) {
            inDegree[edge.toId]++;
        }
    }

    std::queue<std::pair<std::string, int>> q;
    std::unordered_map<std::string, int> ranks;
    int maxRank = 0;

    for (const auto& [id, deg] : inDegree) {
        if (deg == 0) {
            q.push({ id, 0 });
            ranks[id] = 0;
        }
    }

    if (q.empty() && !m_nodes.empty()) {
        q.push({ m_nodes.front()->id, 0 });
        ranks[m_nodes.front()->id] = 0;
    }

    while (!q.empty()) {
        auto [currId, rank] = q.front();
        q.pop();
        maxRank = (std::max)(maxRank, rank);

        auto currNode = FindNode(currId);
        if (currNode && currNode->isExpanded) {
            for (const auto& child : currNode->children) {
                if (child && child->targetOpacity > 0.05f && ranks.find(child->id) == ranks.end()) {
                    ranks[child->id] = rank + 1;
                    q.push({ child->id, rank + 1 });
                }
            }
        }

        for (const auto& edge : m_edges) {
            if (edge.fromId == currId && inDegree.find(edge.toId) != inDegree.end()) {
                if (ranks.find(edge.toId) == ranks.end()) {
                    ranks[edge.toId] = rank + 1;
                    q.push({ edge.toId, rank + 1 });
                }
            }
        }
    }

    for (const auto& n : m_nodes) {
        if (n && n->targetOpacity > 0.05f && ranks.find(n->id) == ranks.end()) {
            ranks[n->id] = 0;
        }
    }

    std::unordered_map<int, std::vector<std::shared_ptr<TopologyNode>>> layers;
    for (const auto& [id, rank] : ranks) {
        auto node = FindNode(id);
        if (node && node->targetOpacity > 0.05f) {
            layers[rank].push_back(node);
        }
    }

    const float levelGap = horizontal ? 240.0f : 140.0f;
    const float siblingGap = horizontal ? 90.0f : 190.0f;

    for (const auto& [rank, layerNodes] : layers) {
        const float totalSpan = (static_cast<float>(layerNodes.size()) - 1.0f) * siblingGap;
        const float startOffset = -totalSpan * 0.5f;

        for (size_t i = 0; i < layerNodes.size(); ++i) {
            auto node = layerNodes[i];
            const float cross = startOffset + static_cast<float>(i) * siblingGap;
            const float along = static_cast<float>(rank) * levelGap;

            if (horizontal) {
                node->targetPos = Point(along + 40.0f, cross + 220.0f);
            } else {
                node->targetPos = Point(cross + 340.0f, along + 40.0f);
            }

            if (node->currentPos.x == 0.0f && node->currentPos.y == 0.0f) {
                node->currentPos = node->targetPos;
            }
        }
    }
}

void TopologyView::ComputeRadialLayout() {
    if (m_nodes.empty()) return;

    auto root = m_nodes.front();
    root->targetPos = Point(360.0f, 240.0f);

    std::vector<std::shared_ptr<TopologyNode>> level1;
    std::vector<std::shared_ptr<TopologyNode>> level2;

    for (size_t i = 1; i < m_nodes.size(); ++i) {
        auto n = m_nodes[i];
        if (!n || n->targetOpacity < 0.05f) continue;
        if (n->parentId == root->id || i <= 6) {
            level1.push_back(n);
        } else {
            level2.push_back(n);
        }
    }

    const float r1 = 180.0f;
    for (size_t i = 0; i < level1.size(); ++i) {
        float angle = static_cast<float>(i) * (2.0f * static_cast<float>(M_PI) / static_cast<float>(level1.size()));
        level1[i]->targetPos = Point(root->targetPos.x + std::cos(angle) * r1,
                                     root->targetPos.y + std::sin(angle) * r1);
    }

    const float r2 = 320.0f;
    for (size_t i = 0; i < level2.size(); ++i) {
        float angle = static_cast<float>(i) * (2.0f * static_cast<float>(M_PI) / static_cast<float>((std::max<size_t>)(1, level2.size())));
        level2[i]->targetPos = Point(root->targetPos.x + std::cos(angle) * r2,
                                     root->targetPos.y + std::sin(angle) * r2);
    }
}

void TopologyView::StepForceDirectedSimulation() {
    const float kRepulsion = 12000.0f;
    const float kSpring = 0.05f;
    const float springLength = 200.0f;
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
        b->velocity.x -= fx;
        b->velocity.y -= fy;
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
    bool keepAnimating = false;

    for (auto& n : m_nodes) {
        if (!n) continue;

        const float posDiffX = n->targetPos.x - n->currentPos.x;
        const float posDiffY = n->targetPos.y - n->currentPos.y;
        const float opacDiff = n->targetOpacity - n->opacity;
        const float scaleDiff = n->targetScale - n->scale;

        if (std::abs(posDiffX) > 0.4f || std::abs(posDiffY) > 0.4f) {
            n->currentPos.x += posDiffX * 0.22f;
            n->currentPos.y += posDiffY * 0.22f;
            keepAnimating = true;
        } else {
            n->currentPos = n->targetPos;
        }

        if (std::abs(opacDiff) > 0.01f) {
            n->opacity += opacDiff * 0.25f;
            keepAnimating = true;
        } else {
            n->opacity = n->targetOpacity;
        }

        if (std::abs(scaleDiff) > 0.01f) {
            n->scale += scaleDiff * 0.25f;
            keepAnimating = true;
        } else {
            n->scale = n->targetScale;
        }

        n->isCulled = (n->opacity <= 0.03f && n->targetOpacity <= 0.0f);
    }

    for (auto& edge : m_edges) {
        const float opacDiff = edge.targetOpacity - edge.opacity;
        if (std::abs(opacDiff) > 0.01f) {
            edge.opacity += opacDiff * 0.25f;
            keepAnimating = true;
        } else {
            edge.opacity = edge.targetOpacity;
        }

        edge.isCulled = (edge.opacity <= 0.03f && edge.targetOpacity <= 0.0f);

        if (m_flowParticlesEnabled && !edge.isCulled) {
            edge.flowPhase = std::fmod(edge.flowPhase + 0.018f * edge.flowSpeed, 1.0f);
            keepAnimating = true;
        }
    }

    m_particleTick += 0.03f;
    m_animating = keepAnimating;

    MarkRenderContentDirty();
    return keepAnimating || m_flowParticlesEnabled;
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
        m_draggingNode = hitNode;
        m_draggingNode->isDragging = true;
        m_lastMousePos = pt;
        m_onNodeClicked.Invoke(this, hitNode);
    } else {
        SetSelectedItem(nullptr);
        m_isPanning = true;
        m_lastMousePos = pt;
    }
    MarkRenderContentDirty();
}

void TopologyView::OnMouseMove(Point pt) {
    if (m_isPanning) {
        m_panOffset.x += (pt.x - m_lastMousePos.x);
        m_panOffset.y += (pt.y - m_lastMousePos.y);
        m_lastMousePos = pt;
        MarkRenderContentDirty();
        return;
    }

    if (m_draggingNode) {
        Point deltaWorld((pt.x - m_lastMousePos.x) / m_zoom, (pt.y - m_lastMousePos.y) / m_zoom);
        m_draggingNode->targetPos.x += deltaWorld.x;
        m_draggingNode->targetPos.y += deltaWorld.y;
        m_draggingNode->currentPos = m_draggingNode->targetPos;
        m_lastMousePos = pt;
        MarkRenderContentDirty();
        return;
    }

    auto hitNode = HitTestNode(pt);
    if (hitNode != m_hoveredNode) {
        if (m_hoveredNode) m_hoveredNode->isHovered = false;
        m_hoveredNode = hitNode;
        if (m_hoveredNode) m_hoveredNode->isHovered = true;
        UpdateHoverHighlights(m_hoveredNode);
        MarkRenderContentDirty();
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
    const float factor = (delta > 0) ? 1.12f : 0.89f;
    const float newZoom = (std::clamp)(m_zoom * factor, 0.25f, 3.0f);

    if (newZoom != m_zoom) {
        m_panOffset.x = center.x - (center.x - m_panOffset.x) * (newZoom / m_zoom);
        m_panOffset.y = center.y - (center.y - m_panOffset.y) * (newZoom / m_zoom);
        m_zoom = newZoom;
        MarkRenderContentDirty();
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
    return false;
}

static void DrawD2DBezier(GraphicsContext& ctx, Point p1, Point cp1, Point cp2, Point p2, D2D1_COLOR_F color, float strokeWidth) {
    auto d2d = ctx.GetD2DContext();
    if (!d2d) return;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (FAILED(d2d->CreateSolidColorBrush(color, &brush))) return;

    Microsoft::WRL::ComPtr<ID2D1Factory> factory;
    d2d->GetFactory(&factory);
    if (!factory) return;

    Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
    if (FAILED(factory->CreatePathGeometry(&path))) return;

    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(path->Open(&sink))) return;

    sink->BeginFigure(D2D1::Point2F(p1.x, p1.y), D2D1_FIGURE_BEGIN_HOLLOW);
    sink->AddBezier(D2D1::BezierSegment(
        D2D1::Point2F(cp1.x, cp1.y),
        D2D1::Point2F(cp2.x, cp2.y),
        D2D1::Point2F(p2.x, p2.y)
    ));
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    sink->Close();

    d2d->DrawGeometry(path.Get(), brush.Get(), strokeWidth);
}

static void FillD2DCircle(GraphicsContext& ctx, Point center, float radius, D2D1_COLOR_F color) {
    auto d2d = ctx.GetD2DContext();
    if (!d2d) return;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(d2d->CreateSolidColorBrush(color, &brush))) {
        d2d->FillEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), radius, radius), brush.Get());
    }
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
    const D2D1_COLOR_F bgColor = isDark ? D2D1::ColorF(0x18181B, 1.0f) : D2D1::ColorF(0xF8F9FA, 1.0f);
    const D2D1_COLOR_F dotColor = isDark ? D2D1::ColorF(0x27272A, 0.85f) : D2D1::ColorF(0xE2E8F0, 0.9f);

    ctx.FillRect(bounds, bgColor);

    const float gridSize = 28.0f * m_zoom;
    const float startX = std::fmod(m_panOffset.x, gridSize);
    const float startY = std::fmod(m_panOffset.y, gridSize);

    for (float x = bounds.x + startX; x < bounds.x + bounds.width; x += gridSize) {
        for (float y = bounds.y + startY; y < bounds.y + bounds.height; y += gridSize) {
            FillD2DCircle(ctx, Point(x, y), 1.2f, dotColor);
        }
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

    for (const auto& edge : m_edges) {
        if (edge.isCulled || edge.opacity <= 0.01f) continue;

        auto from = FindNode(edge.fromId);
        auto to = FindNode(edge.toId);
        if (!from || !to || from->isCulled || to->isCulled) continue;

        const Rect rFrom = GetNodeBounds(from);
        const Rect rTo = GetNodeBounds(to);

        Point p1(rFrom.x + rFrom.width, rFrom.y + rFrom.height * 0.5f);
        Point p2(rTo.x, rTo.y + rTo.height * 0.5f);

        if (m_layoutType == TopologyLayoutType::HierarchicalTopDown) {
            p1 = Point(rFrom.x + rFrom.width * 0.5f, rFrom.y + rFrom.height);
            p2 = Point(rTo.x + rTo.width * 0.5f, rTo.y);
        }

        const float dx = p2.x - p1.x;
        const float dy = p2.y - p1.y;

        Point cp1, cp2;
        if (m_layoutType == TopologyLayoutType::HierarchicalTopDown) {
            cp1 = Point(p1.x, p1.y + (std::max)(30.0f, dy * 0.45f));
            cp2 = Point(p2.x, p2.y - (std::max)(30.0f, dy * 0.45f));
        } else {
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

        DrawD2DBezier(ctx, p1, cp1, cp2, p2, strokeColor, strokeThickness);

        if (m_flowParticlesEnabled && edge.isFlowActive && edge.opacity > 0.3f && !edge.isDimmed) {
            const int particleCount = 2;
            for (int k = 0; k < particleCount; ++k) {
                float t = std::fmod(edge.flowPhase + static_cast<float>(k) * 0.5f, 1.0f);
                float u = 1.0f - t;
                float tt = t * t;
                float uu = u * u;
                float uuu = uu * u;
                float ttt = tt * t;

                Point particlePos(
                    uuu * p1.x + 3.0f * uu * t * cp1.x + 3.0f * u * tt * cp2.x + ttt * p2.x,
                    uuu * p1.y + 3.0f * uu * t * cp1.y + 3.0f * u * tt * cp2.y + ttt * p2.y
                );

                D2D1_COLOR_F glowColor = strokeColor;
                glowColor.a = 0.9f * edge.opacity;
                FillD2DCircle(ctx, particlePos, 3.2f * m_zoom, glowColor);
            }
        }

        if (!edge.label.empty() && edge.opacity > 0.4f && m_zoom > 0.6f) {
            Point mid(
                0.125f * p1.x + 0.375f * cp1.x + 0.375f * cp2.x + 0.125f * p2.x,
                0.125f * p1.y + 0.375f * cp1.y + 0.375f * cp2.y + 0.125f * p2.y
            );
            const float labelW = 60.0f * m_zoom;
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

        D2D1_COLOR_F cardBg = isDark ? D2D1::ColorF(0x242429, 0.92f) : D2D1::ColorF(0xFFFFFF, 0.96f);
        if (node->isHighlighted) {
            cardBg = isDark ? D2D1::ColorF(0x2D3748, 0.95f) : D2D1::ColorF(0xF0F7FF, 0.98f);
        }
        cardBg.a *= node->opacity;
        if (node->isDimmed) cardBg.a *= 0.35f;

        if (node->isSelected && node->opacity > 0.5f) {
            Rect glowRect(cardRect.x - 3.0f * m_zoom, cardRect.y - 3.0f * m_zoom,
                          cardRect.width + 6.0f * m_zoom, cardRect.height + 6.0f * m_zoom);
            D2D1_COLOR_F glowColor = D2D1::ColorF(0x3B82F6, 0.45f * node->opacity);
            ctx.DrawRoundedRect(glowRect, radius + 2.0f, glowColor, 3.0f * m_zoom);
        }

        ctx.FillRoundedRect(cardRect, radius, cardBg);

        D2D1_COLOR_F borderColor = node->customColor.value_or(
            node->isSelected ? D2D1::ColorF(0x3B82F6, 1.0f) :
            (node->isHovered ? (isDark ? D2D1::ColorF(0x60A5FA, 0.8f) : D2D1::ColorF(0x3B82F6, 0.8f)) :
            (isDark ? D2D1::ColorF(0x3F3F46, 0.8f) : D2D1::ColorF(0xCBD5E1, 0.85f)))
        );
        borderColor.a *= node->opacity;
        if (node->isDimmed) borderColor.a *= 0.3f;

        ctx.DrawRoundedRect(cardRect, radius, borderColor, (node->isSelected ? 2.0f : 1.0f) * m_zoom);

        const D2D1_COLOR_F statusColor = GetStatusColor(node->status, isDark);
        Point statusDotPos(cardRect.x + 14.0f * m_zoom * node->scale, cardRect.y + 16.0f * m_zoom * node->scale);
        
        if (node->status == TopologyNodeStatus::Error || node->status == TopologyNodeStatus::Processing) {
            float pulseScale = 1.0f + 0.3f * std::sin(m_particleTick * 3.0f);
            D2D1_COLOR_F haloColor = statusColor;
            haloColor.a = 0.35f * node->opacity;
            FillD2DCircle(ctx, statusDotPos, 6.0f * m_zoom * pulseScale, haloColor);
        }
        FillD2DCircle(ctx, statusDotPos, 4.0f * m_zoom, statusColor);

        float textLeft = cardRect.x + 26.0f * m_zoom * node->scale;
        if (!node->icon.empty()) {
            Rect iconRect(textLeft, cardRect.y + 8.0f * m_zoom * node->scale, 20.0f * m_zoom, 20.0f * m_zoom);
            D2D1_COLOR_F iconColor = isDark ? D2D1::ColorF(0xFAFAFA, node->opacity) : D2D1::ColorF(0x0F172A, node->opacity);
            if (node->isDimmed) iconColor.a *= 0.35f;
            ctx.DrawText(node->icon, iconRect, iconColor, "Segoe UI Emoji", 13.0f * m_zoom * node->scale,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            textLeft += 22.0f * m_zoom * node->scale;
        }

        Rect titleRect(textLeft, cardRect.y + 8.0f * m_zoom * node->scale, cardRect.width - (textLeft - cardRect.x) - 10.0f * m_zoom, 20.0f * m_zoom * node->scale);
        D2D1_COLOR_F titleColor = isDark ? D2D1::ColorF(0xF4F4F5, node->opacity) : D2D1::ColorF(0x0F172A, node->opacity);
        if (node->isDimmed) titleColor.a *= 0.35f;
        ctx.DrawText(node->label, titleRect, titleColor, "Segoe UI", 12.0f * m_zoom * node->scale,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);

        if (!node->subLabel.empty() || !node->category.empty()) {
            std::string sub = !node->subLabel.empty() ? node->subLabel : node->category;
            Rect subRect(cardRect.x + 14.0f * m_zoom * node->scale, cardRect.y + 34.0f * m_zoom * node->scale,
                         cardRect.width - 28.0f * m_zoom, 16.0f * m_zoom * node->scale);
            D2D1_COLOR_F subColor = isDark ? D2D1::ColorF(0xA1A1AA, 0.9f * node->opacity) : D2D1::ColorF(0x64748B, 0.9f * node->opacity);
            if (node->isDimmed) subColor.a *= 0.35f;
            ctx.DrawText(sub, subRect, subColor, "Segoe UI", 10.0f * m_zoom * node->scale,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        if (!node->badge.empty() && m_zoom > 0.65f) {
            const float badgeW = 60.0f * m_zoom * node->scale;
            const float badgeH = 16.0f * m_zoom * node->scale;
            Rect badgeRect(cardRect.x + cardRect.width - badgeW - 8.0f * m_zoom, cardRect.y + 8.0f * m_zoom, badgeW, badgeH);

            D2D1_COLOR_F badgeBg = isDark ? D2D1::ColorF(0x3F3F46, 0.6f * node->opacity) : D2D1::ColorF(0xE2E8F0, 0.8f * node->opacity);
            ctx.FillRoundedRect(badgeRect, 3.0f * m_zoom, badgeBg);

            D2D1_COLOR_F badgeTextColor = isDark ? D2D1::ColorF(0xD4D4D8, node->opacity) : D2D1::ColorF(0x475569, node->opacity);
            ctx.DrawText(node->badge, badgeRect, badgeTextColor, "Segoe UI", 9.0f * m_zoom * node->scale,
                         DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

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

    const float hudW = 190.0f;
    const float hudH = 34.0f;
    Rect hudRect(bounds.x + bounds.width - hudW - 14.0f, bounds.y + bounds.height - hudH - 14.0f, hudW, hudH);

    D2D1_COLOR_F hudBg = isDark ? D2D1::ColorF(0x1F1F23, 0.88f) : D2D1::ColorF(0xFFFFFF, 0.92f);
    D2D1_COLOR_F hudBorder = isDark ? D2D1::ColorF(0x3F3F46, 0.7f) : D2D1::ColorF(0xCBD5E1, 0.8f);

    ctx.FillRoundedRect(hudRect, 6.0f, hudBg);
    ctx.DrawRoundedRect(hudRect, 6.0f, hudBorder, 1.0f);

    std::string zoomText = std::format("{:.0f}%", m_zoom * 100.0f);
    D2D1_COLOR_F textColor = isDark ? D2D1::ColorF(0xD4D4D8, 1.0f) : D2D1::ColorF(0x334155, 1.0f);

    ctx.DrawText("Zoom: " + zoomText + " | [F] 适配 | [R] 重置", hudRect, textColor, "Segoe UI", 11.0f,
                 DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

} // namespace CUI
