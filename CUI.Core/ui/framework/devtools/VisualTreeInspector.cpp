#include "VisualTreeInspector.h"
#include "DevToolsManager.h"
#include "../window/Window.h"
#include "../controls/NavigationViewItem.h"

#include <cstdio>

namespace CUI {

static std::string ExtractElementSummary(UIElement* el) {
    if (!el) return "";
    std::string summary;
    if (!el->GetId().empty()) {
        summary = "#" + el->GetId() + " ";
    }

    if (auto* nvi = dynamic_cast<NavigationViewItem*>(el)) {
        if (!nvi->GetContent().empty()) {
            return summary + "\"" + nvi->GetContent() + "\"";
        }
    }
    if (auto* nvih = dynamic_cast<NavigationViewItemHeader*>(el)) {
        if (!nvih->GetText().empty()) {
            return summary + "\"" + nvih->GetText() + "\"";
        }
    }
    const std::string& text = el->GetText();
    if (!text.empty()) {
        std::string preview = text;
        if (preview.length() > 24) {
            preview = preview.substr(0, 21) + "...";
        }
        return summary + "\"" + preview + "\"";
    }
    if (!summary.empty()) {
        summary.pop_back();
        return summary;
    }
    return "";
}

VisualTreeInspector::VisualTreeInspector() = default;

void VisualTreeInspector::RebuildTree(UIElement* root) {
    if (!root) {
        m_treeRoot = nullptr;
        m_flatNodes.clear();
        return;
    }

    m_treeRoot = std::make_shared<VisualTreeNode>();
    m_treeRoot->rawElement = root;
    m_treeRoot->typeName = ExtractElementTypeName(root);
    m_treeRoot->summary = ExtractElementSummary(root);
    m_treeRoot->depth = 0;
    m_treeRoot->bounds = root->GetBounds();
    m_treeRoot->visibility = root->GetVisibility();

    auto it = m_expandedStates.find(root);
    m_treeRoot->isExpanded = (it != m_expandedStates.end()) ? it->second : true;
    m_treeRoot->isSelected = (root == m_selectedElement);

    BuildNodeRecursive(root, m_treeRoot, 1);
    FlattenTree();
}

void VisualTreeInspector::BuildNodeRecursive(UIElement* el, std::shared_ptr<VisualTreeNode> parentNode, int depth) {
    if (!el) return;

    for (const auto& child : el->GetChildren()) {
        if (!child) continue;

        auto childNode = std::make_shared<VisualTreeNode>();
        childNode->rawElement = child.get();
        childNode->typeName = ExtractElementTypeName(child.get());
        childNode->summary = ExtractElementSummary(child.get());
        childNode->depth = depth;
        childNode->bounds = child->GetBounds();
        childNode->visibility = child->GetVisibility();

        auto it = m_expandedStates.find(child.get());
        childNode->isExpanded = (it != m_expandedStates.end()) ? it->second : (depth < 2);
        childNode->isSelected = (child.get() == m_selectedElement);

        parentNode->children.push_back(childNode);
        BuildNodeRecursive(child.get(), childNode, depth + 1);
    }
}

void VisualTreeInspector::FlattenTree() {
    m_flatNodes.clear();
    if (!m_treeRoot) return;

    std::function<void(const std::shared_ptr<VisualTreeNode>&)> walk =
        [&](const std::shared_ptr<VisualTreeNode>& node) {
            if (!node) return;

            // 过滤折叠模式下，如果节点处于折叠状态且未被选中，则不加入平铺展示列表
            if (m_hideCollapsed && node->visibility == Visibility::Collapsed && node->rawElement != m_selectedElement) {
                return;
            }

            FlatTreeNode item;
            item.node = node;
            item.depth = node->depth;
            item.height = 24.0f;
            item.y = static_cast<float>(m_flatNodes.size()) * 24.0f;
            m_flatNodes.push_back(item);

            if (node->isExpanded) {
                for (const auto& c : node->children) {
                    walk(c);
                }
            }
        };

    walk(m_treeRoot);

    const float visibleHeight = m_panelRect.height - 72.0f;
    const float totalContentHeight = static_cast<float>(m_flatNodes.size()) * 24.0f;
    m_maxTreeScroll = (std::max)(0.0f, totalContentHeight - visibleHeight);
    m_treeScrollY = std::clamp(m_treeScrollY, 0.0f, m_maxTreeScroll);
}

void VisualTreeInspector::ScrollToElement(UIElement* element, bool forceCenter) {
    if (!element) return;
    const float visibleHeight = m_panelRect.height - 72.0f;
    for (const auto& item : m_flatNodes) {
        if (item.node && item.node->rawElement == element) {
            const float itemTop = item.y;
            const float itemBottom = item.y + item.height;
            const bool isFullyVisible = (itemTop >= m_treeScrollY + 4.0f && itemBottom <= m_treeScrollY + visibleHeight - 4.0f);
            if (forceCenter || !isFullyVisible) {
                const float targetScroll = item.y - (visibleHeight * 0.5f) + (item.height * 0.5f);
                m_treeScrollY = std::clamp(targetScroll, 0.0f, m_maxTreeScroll);
            }
            if (Window::Current()) {
                Window::Current()->InvalidateLogicalRect(m_panelRect);
            }
            break;
        }
    }
}

void VisualTreeInspector::SelectElement(UIElement* element, bool forceCenter) {
    m_selectedElement = element;
    if (element && Window::Current()) {
        // 自动展开祖先链路
        UIElement* curr = element;
        while (curr) {
            m_expandedStates[curr] = true;
            curr = curr->GetParent();
        }
        RebuildTree(Window::Current()->GetRootElement().get());
        ScrollToElement(element, forceCenter);
    }
}

bool VisualTreeInspector::OnMouseMove(Point pt) {
    if (m_isDragging) {
        m_panelRect.x = pt.x - m_dragOffset.x;
        m_panelRect.y = pt.y - m_dragOffset.y;
        if (Window::Current()) {
            Window::Current()->InvalidateLogicalRect(Rect(0, 0, 10000, 10000));
        }
        return true;
    }

    if (m_isDraggingScrollbar && m_maxTreeScroll > 0.0f) {
        constexpr float visibleHeight = 520.0f - 80.0f;
        const float totalContentHeight = static_cast<float>(m_flatNodes.size()) * 24.0f;
        const float thumbHeight = (std::max)(24.0f, (visibleHeight / totalContentHeight) * visibleHeight);
        const float trackHeight = visibleHeight - thumbHeight;

        if (trackHeight > 0.0f) {
            float deltaY = pt.y - m_dragScrollStartY;
            float scrollDelta = (deltaY / trackHeight) * m_maxTreeScroll;
            m_treeScrollY = std::clamp(m_dragScrollStartVal + scrollDelta, 0.0f, m_maxTreeScroll);
            if (Window::Current()) {
                Window::Current()->InvalidateLogicalRect(m_panelRect);
            }
        }
        return true;
    }

    return ContainsPoint(pt);
}

bool VisualTreeInspector::OnMouseDown(Point pt) {
    if (!ContainsPoint(pt)) return false;

    // 1. 标题栏区域判定
    const Rect titleRect(m_panelRect.x, m_panelRect.y, m_panelRect.width, 36.0f);
    if (titleRect.Contains(pt.x, pt.y)) {
        // 工具栏按钮坐标
        float bx = m_panelRect.x + m_panelRect.width - 34.0f;

        // [✖ 关闭]
        if (Rect(bx, m_panelRect.y + 4.0f, 28.0f, 28.0f).Contains(pt.x, pt.y)) {
            DevToolsManager::Instance().SetInspectorVisible(false);
            return true;
        }
        bx -= 76.0f;
        // [重绘热力]
        if (Rect(bx, m_panelRect.y + 4.0f, 72.0f, 28.0f).Contains(pt.x, pt.y)) {
            DevToolsManager::Instance().ToggleOverdrawVisualizer();
            return true;
        }
        bx -= 64.0f;
        // [性能 HUD]
        if (Rect(bx, m_panelRect.y + 4.0f, 60.0f, 28.0f).Contains(pt.x, pt.y)) {
            DevToolsManager::Instance().TogglePerformanceOverlay();
            return true;
        }
        bx -= 64.0f;
        // [主题切换]
        if (Rect(bx, m_panelRect.y + 4.0f, 60.0f, 28.0f).Contains(pt.x, pt.y)) {
            auto curMode = ThemeManager::Instance().GetThemeMode();
            auto newMode = (curMode == ThemeMode::Dark) ? ThemeMode::Light : ThemeMode::Dark;
            ThemeManager::Instance().ApplyThemeMode(newMode);
            if (Window::Current()) {
                Window::Current()->ApplyThemeMode(newMode);
            }
            return true;
        }
        bx -= 76.0f;
        // [拾取元素]
        if (Rect(bx, m_panelRect.y + 4.0f, 72.0f, 28.0f).Contains(pt.x, pt.y)) {
            DevToolsManager::Instance().ToggleInspectMode();
            return true;
        }
        bx -= 70.0f;
        // [过滤折叠]
        if (Rect(bx, m_panelRect.y + 4.0f, 66.0f, 28.0f).Contains(pt.x, pt.y)) {
            m_hideCollapsed = !m_hideCollapsed;
            FlattenTree();
            ScrollToElement(m_selectedElement, false);
            if (Window::Current()) {
                Window::Current()->InvalidateLogicalRect(m_panelRect);
            }
            return true;
        }

        // 其它部分响应面板拖动
        m_isDragging = true;
        m_dragOffset = Point(pt.x - m_panelRect.x, pt.y - m_panelRect.y);
        return true;
    }

    // 2. 滚动条点击判定
    const Rect scrollTrack(m_panelRect.x + 348.0f, m_panelRect.y + 64.0f, 10.0f, m_panelRect.height - 72.0f);
    if (scrollTrack.Contains(pt.x, pt.y) && m_maxTreeScroll > 0.0f) {
        m_isDraggingScrollbar = true;
        m_dragScrollStartY = pt.y;
        m_dragScrollStartVal = m_treeScrollY;
        return true;
    }

    // 3. 左侧元素树区域点击
    const Rect treeViewRect(m_panelRect.x + 8.0f, m_panelRect.y + 64.0f, 340.0f, m_panelRect.height - 72.0f);
    if (treeViewRect.Contains(pt.x, pt.y)) {
        float relY = pt.y - treeViewRect.y + m_treeScrollY;
        size_t index = static_cast<size_t>(relY / 24.0f);

        if (index < m_flatNodes.size()) {
            auto& item = m_flatNodes[index];
            if (item.node && item.node->rawElement) {
                // 点击左侧展开折叠小箭头区域
                const float arrowX = treeViewRect.x + static_cast<float>(item.depth) * 16.0f;
                if (pt.x >= arrowX && pt.x <= arrowX + 18.0f && !item.node->children.empty()) {
                    item.node->isExpanded = !item.node->isExpanded;
                    m_expandedStates[item.node->rawElement] = item.node->isExpanded;
                    FlattenTree();
                } else {
                    SelectElement(item.node->rawElement, false);
                }
                if (Window::Current()) {
                    Window::Current()->InvalidateLogicalRect(Rect(0, 0, 10000, 10000));
                }
                return true;
            }
        }
    }

    return true; // 拦截 panel 内部点击
}

bool VisualTreeInspector::OnMouseUp(Point /*pt*/) {
    if (m_isDragging || m_isDraggingScrollbar) {
        m_isDragging = false;
        m_isDraggingScrollbar = false;
        return true;
    }
    return false;
}

bool VisualTreeInspector::OnMouseWheel(Point pt, float delta) {
    // 整个左侧分栏 (从 x 到 x + 360, 标题栏以下) 均响应滚轮
    const Rect leftPaneRect(m_panelRect.x, m_panelRect.y + 36.0f, 360.0f, m_panelRect.height - 36.0f);
    if (leftPaneRect.Contains(pt.x, pt.y)) {
        // 每滚一格滑动 60 逻辑像素 (约 2.5 行)，丝滑且快速！
        m_treeScrollY -= delta * 60.0f;
        m_treeScrollY = std::clamp(m_treeScrollY, 0.0f, m_maxTreeScroll);
        if (Window::Current()) {
            Window::Current()->InvalidateLogicalRect(m_panelRect);
        }
        return true;
    }
    return false;
}

void VisualTreeInspector::Render(GraphicsContext& ctx, Size /*windowSize*/) {
    // 裁剪使整个面板具有 8px 圆角外轮廓 (标题栏顶部与面板底部的圆角自然由此生成，而标题栏底部与分割线贴合处为平直 90 度直角，无凹陷内缩圆角)
    ctx.PushRoundedClip(m_panelRect, 8.0f);

    // 1. 面板底盘 (半透明亚克力深色玻璃)
    ctx.FillRect(m_panelRect, Color(0.11f, 0.13f, 0.17f, 0.96f));

    // 2. 标题栏 (矩形填充，左下右下无圆角平直贴合横向分界面)
    const Rect titleRect(m_panelRect.x, m_panelRect.y, m_panelRect.width, 36.0f);
    ctx.FillRect(titleRect, Color(0.08f, 0.09f, 0.12f, 0.95f));
    ctx.DrawLine(
        Point(m_panelRect.x, m_panelRect.y + 36.0f),
        Point(m_panelRect.x + m_panelRect.width, m_panelRect.y + 36.0f),
        Color(0.24f, 0.28f, 0.35f, 0.70f),
        1.0f);

    // 标题文字 (中文)
    ctx.DrawText(
        "🛠  CUI 运行时元素树与检查器",
        Rect(m_panelRect.x + 12.0f, m_panelRect.y, 220.0f, 36.0f),
        Color(0.92f, 0.94f, 0.98f, 1.0f),
        "Segoe UI",
        12.0f,
        TextAlignment::Left,
        TextVerticalAlignment::Center,
        FontWeight::Bold);

    // 3. 工具栏按钮 (中文)
    float bx = m_panelRect.x + m_panelRect.width - 34.0f;

    // [✖ 关闭]
    ctx.DrawText("✕", Rect(bx, m_panelRect.y + 4.0f, 26.0f, 28.0f),
                 Color(0.85f, 0.85f, 0.85f, 0.9f), "Segoe UI", 12.0f, TextAlignment::Center, TextVerticalAlignment::Center);

    bx -= 76.0f;
    // [重绘热力]
    bool overdrawActive = DevToolsManager::Instance().IsOverdrawVisualizerEnabled();
    ctx.FillRoundedRect(Rect(bx, m_panelRect.y + 5.0f, 70.0f, 26.0f), 4.0f,
                        overdrawActive ? Color(0.85f, 0.35f, 0.25f, 0.85f) : Color(0.18f, 0.21f, 0.27f, 0.90f));
    ctx.DrawText("重绘热力", Rect(bx, m_panelRect.y + 5.0f, 70.0f, 26.0f),
                 Color(0.95f, 0.95f, 0.95f, 1.0f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center, FontWeight::SemiBold);

    bx -= 64.0f;
    // [性能 HUD]
    bool hudActive = DevToolsManager::Instance().IsPerformanceOverlayEnabled();
    ctx.FillRoundedRect(Rect(bx, m_panelRect.y + 5.0f, 58.0f, 26.0f), 4.0f,
                        hudActive ? Color(0.20f, 0.65f, 0.40f, 0.85f) : Color(0.18f, 0.21f, 0.27f, 0.90f));
    ctx.DrawText("性能HUD", Rect(bx, m_panelRect.y + 5.0f, 58.0f, 26.0f),
                 Color(0.95f, 0.95f, 0.95f, 1.0f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center, FontWeight::SemiBold);

    bx -= 64.0f;
    // [主题切换]
    bool isDark = (ThemeManager::Instance().GetThemeMode() == ThemeMode::Dark);
    ctx.FillRoundedRect(Rect(bx, m_panelRect.y + 5.0f, 58.0f, 26.0f), 4.0f, Color(0.18f, 0.21f, 0.27f, 0.90f));
    ctx.DrawText(isDark ? "🌙 深色" : "☀️ 浅色", Rect(bx, m_panelRect.y + 5.0f, 58.0f, 26.0f),
                 Color(0.95f, 0.95f, 0.95f, 1.0f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center);

    bx -= 76.0f;
    // [拾取元素]
    bool inspectActive = DevToolsManager::Instance().IsInspectMode();
    ctx.FillRoundedRect(Rect(bx, m_panelRect.y + 5.0f, 70.0f, 26.0f), 4.0f,
                        inspectActive ? Color(0.22f, 0.55f, 0.92f, 0.90f) : Color(0.18f, 0.21f, 0.27f, 0.90f));
    ctx.DrawText("🔍 拾取", Rect(bx, m_panelRect.y + 5.0f, 70.0f, 26.0f),
                 Color(0.95f, 0.95f, 0.95f, 1.0f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center, FontWeight::SemiBold);

    bx -= 70.0f;
    // [过滤折叠]
    ctx.FillRoundedRect(Rect(bx, m_panelRect.y + 5.0f, 66.0f, 26.0f), 4.0f,
                        m_hideCollapsed ? Color(0.22f, 0.55f, 0.92f, 0.90f) : Color(0.18f, 0.21f, 0.27f, 0.90f));
    ctx.DrawText(m_hideCollapsed ? "隐藏折叠" : "过滤折叠", Rect(bx, m_panelRect.y + 5.0f, 66.0f, 26.0f),
                 Color(0.95f, 0.95f, 0.95f, 1.0f), "Segoe UI", 10.0f, TextAlignment::Center, TextVerticalAlignment::Center, FontWeight::SemiBold);

    // 4. 分栏布局 (左 360px: 元素树，右 380px: 盒模型与属性面板)
    const float splitX = m_panelRect.x + 360.0f;
    ctx.DrawLine(
        Point(splitX, m_panelRect.y + 36.0f),
        Point(splitX, m_panelRect.y + m_panelRect.height),
        Color(0.24f, 0.28f, 0.35f, 0.70f),
        1.0f);

    // --- 左侧：元素树 ---
    ctx.DrawText(
        "可视化元素树 (VISUAL TREE)",
        Rect(m_panelRect.x + 12.0f, m_panelRect.y + 42.0f, 240.0f, 18.0f),
        Color(0.60f, 0.65f, 0.72f, 1.0f),
        "Segoe UI",
        10.0f,
        TextAlignment::Left,
        TextVerticalAlignment::Center,
        FontWeight::Bold);

    const Rect treeViewRect(m_panelRect.x + 8.0f, m_panelRect.y + 64.0f, 338.0f, m_panelRect.height - 72.0f);
    ctx.PushClip(treeViewRect);

    float curY = treeViewRect.y - m_treeScrollY;
    for (const auto& item : m_flatNodes) {
        if (!item.node) continue;
        if (curY + 24.0f >= treeViewRect.y && curY <= treeViewRect.y + treeViewRect.height) {
            const Rect rowRect(treeViewRect.x, curY, treeViewRect.width, 24.0f);

            // 选中高亮行
            if (item.node->rawElement == m_selectedElement) {
                ctx.FillRoundedRect(rowRect, 4.0f, Color(0.18f, 0.38f, 0.68f, 0.70f));
                ctx.DrawRoundedRect(rowRect, 4.0f, Color(0.35f, 0.60f, 0.95f, 0.85f), 1.0f);
            }

            const float indent = static_cast<float>(item.depth) * 14.0f;
            const float arrowX = treeViewRect.x + 4.0f + indent;

            // 展开折叠箭头
            if (!item.node->children.empty()) {
                ctx.DrawText(
                    item.node->isExpanded ? "▼" : "▶",
                    Rect(arrowX, curY, 14.0f, 24.0f),
                    Color(0.65f, 0.70f, 0.78f, 1.0f),
                    "Segoe UI",
                    8.5f,
                    TextAlignment::Center,
                    TextVerticalAlignment::Center);
            }

            // 节点名称与概要信息
            std::string label = item.node->typeName;
            if (!item.node->summary.empty()) {
                label += " " + item.node->summary;
            }

            if (item.node->visibility == Visibility::Collapsed) {
                label += "  [折叠 0×0]";
            } else if (item.node->visibility == Visibility::Hidden) {
                label += "  [隐藏]";
            } else {
                char dimStr[64];
                std::snprintf(dimStr, sizeof(dimStr), "  %.0f×%.0f", item.node->bounds.width, item.node->bounds.height);
                label += dimStr;
            }

            Color textColor;
            if (item.node->rawElement == m_selectedElement) {
                textColor = Color(0.98f, 0.98f, 1.0f, 1.0f);
            } else if (item.node->visibility == Visibility::Collapsed || item.node->visibility == Visibility::Hidden) {
                textColor = Color(0.55f, 0.58f, 0.65f, 0.60f);
            } else {
                textColor = Color(0.85f, 0.88f, 0.92f, 0.95f);
            }

            ctx.DrawText(
                label,
                Rect(arrowX + 16.0f, curY, treeViewRect.width - (arrowX - treeViewRect.x) - 10.0f, 24.0f),
                textColor,
                "Segoe UI",
                11.0f,
                TextAlignment::Left,
                TextVerticalAlignment::Center,
                (item.node->rawElement == m_selectedElement) ? FontWeight::SemiBold : FontWeight::Normal);
        }
        curY += 24.0f;
    }
    ctx.PopClip();

    // 绘制左侧滚动条 (Track + Thumb)
    if (m_maxTreeScroll > 0.0f) {
        const float trackX = m_panelRect.x + 350.0f;
        const float trackY = treeViewRect.y;
        const float trackH = treeViewRect.height;
        ctx.FillRoundedRect(Rect(trackX, trackY, 4.0f, trackH), 2.0f, Color(0.18f, 0.22f, 0.28f, 0.50f));

        const float totalContentHeight = static_cast<float>(m_flatNodes.size()) * 24.0f;
        const float thumbH = (std::max)(24.0f, (trackH / totalContentHeight) * trackH);
        const float thumbY = trackY + (m_treeScrollY / m_maxTreeScroll) * (trackH - thumbH);
        ctx.FillRoundedRect(Rect(trackX, thumbY, 4.0f, thumbH), 2.0f, Color(0.48f, 0.55f, 0.65f, 0.85f));
    }

    // --- 右侧：属性与盒模型图解 ---
    const float rightX = splitX + 12.0f;
    const float rightW = m_panelRect.width - (splitX - m_panelRect.x) - 24.0f;

    ctx.DrawText(
        "属性与盒模型 (PROPERTIES & BOX MODEL)",
        Rect(rightX, m_panelRect.y + 42.0f, 280.0f, 18.0f),
        Color(0.60f, 0.65f, 0.72f, 1.0f),
        "Segoe UI",
        10.0f,
        TextAlignment::Left,
        TextVerticalAlignment::Center,
        FontWeight::Bold);

    if (m_selectedElement) {
        BoxModelMetrics metrics = BoxModelOverlay::ExtractMetrics(m_selectedElement);

        // 1. 类型名与地址标识卡片
        const Rect infoCard(rightX, m_panelRect.y + 66.0f, rightW, 46.0f);
        ctx.FillRoundedRect(infoCard, 5.0f, Color(0.08f, 0.10f, 0.13f, 0.85f));
        ctx.DrawRoundedRect(infoCard, 5.0f, Color(0.24f, 0.28f, 0.35f, 0.60f), 1.0f);

        std::string cardTitle = metrics.typeName;
        std::string summary = ExtractElementSummary(m_selectedElement);
        if (!summary.empty()) {
            cardTitle += " " + summary;
        }

        ctx.DrawText(
            cardTitle,
            Rect(rightX + 10.0f, infoCard.y + 4.0f, rightW - 20.0f, 20.0f),
            Color(0.38f, 0.76f, 0.99f, 1.0f),
            "Segoe UI",
            12.0f,
            TextAlignment::Left,
            TextVerticalAlignment::Center,
            FontWeight::Bold);

        char metaBuf[128];
        std::snprintf(metaBuf, sizeof(metaBuf), "实例地址: %s  |  子元素数: %zu  |  启用: %s",
                      metrics.elementAddress.c_str(), metrics.childCount, metrics.isEnabled ? "是" : "否");
        ctx.DrawText(
            metaBuf,
            Rect(rightX + 10.0f, infoCard.y + 24.0f, rightW - 20.0f, 18.0f),
            Color(0.65f, 0.70f, 0.78f, 0.90f),
            "Segoe UI",
            9.5f,
            TextAlignment::Left,
            TextVerticalAlignment::Center);

        // 2. 盒模型同心架构图
        const Rect diagramRect(rightX, infoCard.y + infoCard.height + 12.0f, rightW, 190.0f);
        BoxModelOverlay::RenderDiagram(ctx, metrics, diagramRect);

        // 3. 几何参数列表卡片 (中文)
        const Rect detailCard(rightX, diagramRect.y + diagramRect.height + 12.0f, rightW, 140.0f);
        ctx.FillRoundedRect(detailCard, 5.0f, Color(0.08f, 0.10f, 0.13f, 0.85f));
        ctx.DrawRoundedRect(detailCard, 5.0f, Color(0.24f, 0.28f, 0.35f, 0.60f), 1.0f);

        char line1[128], line2[128], line3[128], line4[128];
        if (metrics.visibility == Visibility::Collapsed) {
            std::snprintf(line1, sizeof(line1), "物理边界:  0×0 (已折叠未排版)");
        } else {
            std::snprintf(line1, sizeof(line1), "物理边界:  x: %.1f, y: %.1f, 宽: %.1f, 高: %.1f",
                          metrics.bounds.x, metrics.bounds.y, metrics.bounds.width, metrics.bounds.height);
        }
        std::snprintf(line2, sizeof(line2), "外边距:    左: %.0f, 上: %.0f, 右: %.0f, 下: %.0f",
                      metrics.margin.left, metrics.margin.top, metrics.margin.right, metrics.margin.bottom);
        std::snprintf(line3, sizeof(line3), "内边距:    左: %.0f, 上: %.0f, 右: %.0f, 下: %.0f",
                      metrics.padding.left, metrics.padding.top, metrics.padding.right, metrics.padding.bottom);
        const char* visStr = (metrics.visibility == Visibility::Visible) ? "可见 (Visible)" :
                             (metrics.visibility == Visibility::Collapsed) ? "折叠 (Collapsed)" : "隐藏 (Hidden)";
        std::snprintf(line4, sizeof(line4), "边框粗细:  %.1f px  |  可见性: %s",
                      metrics.borderThickness, visStr);

        float dy = detailCard.y + 8.0f;
        for (const char* l : { line1, line2, line3, line4 }) {
            ctx.DrawText(
                l,
                Rect(rightX + 12.0f, dy, rightW - 24.0f, 24.0f),
                Color(0.85f, 0.88f, 0.92f, 0.95f),
                "Segoe UI",
                10.5f,
                TextAlignment::Left,
                TextVerticalAlignment::Center);
            dy += 28.0f;
        }
    } else {
        ctx.DrawText(
            "请在左侧点击任意元素，或点击上方【🔍 拾取】在界面直接审查。",
            Rect(rightX, m_panelRect.y + 120.0f, rightW, 60.0f),
            Color(0.60f, 0.65f, 0.72f, 0.85f),
            "Segoe UI",
            12.0f,
            TextAlignment::Center,
            TextVerticalAlignment::Center);
    }

    // 结束圆角裁切
    ctx.PopClip();

    // 绘制外边框 (在 PopClip 之后绘制，保证 1px 边框完整无损)
    ctx.DrawRoundedRect(m_panelRect, 8.0f, Color(0.28f, 0.33f, 0.40f, 0.85f), 1.0f);
}

} // namespace CUI
