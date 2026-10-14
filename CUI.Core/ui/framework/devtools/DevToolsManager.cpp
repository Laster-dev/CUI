#include "DevToolsManager.h"
#include "../window/Window.h"

namespace CUI {

DevToolsManager& DevToolsManager::Instance() {
    static DevToolsManager s_instance;
    return s_instance;
}

DevToolsManager::DevToolsManager() = default;

void DevToolsManager::SetInspectorVisible(bool visible) {
    if (m_inspectorVisible == visible) return;
    m_inspectorVisible = visible;
    if (m_inspectorVisible && Window::Current()) {
        m_inspector.RebuildTree(Window::Current()->GetRootElement().get());
    }
    if (Window::Current()) {
        Window::Current()->InvalidateLogicalRect(Rect(0, 0, 10000, 10000));
    }
}

void DevToolsManager::ToggleDevTools() {
    SetInspectorVisible(!m_inspectorVisible);
}

void DevToolsManager::SetInspectMode(bool active) {
    m_inspectMode = active;
    if (!active) {
        m_hoveredInspectElement = nullptr;
    }
    if (Window::Current()) {
        Window::Current()->InvalidateLogicalRect(Rect(0, 0, 10000, 10000));
    }
}

void DevToolsManager::ToggleInspectMode() {
    SetInspectMode(!m_inspectMode);
}

void DevToolsManager::SetOverdrawVisualizerEnabled(bool enabled) {
    m_overdrawVisualizerEnabled = enabled;
    if (!enabled) {
        m_overdrawVisualizer.Clear();
    }
    if (Window::Current()) {
        Window::Current()->InvalidateLogicalRect(Rect(0, 0, 10000, 10000));
    }
}

void DevToolsManager::ToggleOverdrawVisualizer() {
    SetOverdrawVisualizerEnabled(!m_overdrawVisualizerEnabled);
}

void DevToolsManager::SelectElement(UIElement* element, bool forceCenter) {
    m_inspector.SelectElement(element, forceCenter);
    if (Window::Current()) {
        Window::Current()->InvalidateLogicalRect(Rect(0, 0, 10000, 10000));
    }
}

bool DevToolsManager::OnKeyDown(int vkCode, bool ctrl, bool shift, bool /*alt*/) {
    // 1. F12: 切换检查面板显隐
    if (vkCode == VK_F12) {
        ToggleDevTools();
        return true;
    }

    // 2. Ctrl + Shift + I: 切换检查面板
    if (ctrl && shift && (vkCode == 'I' || vkCode == 'i')) {
        ToggleDevTools();
        return true;
    }

    // 3. Ctrl + Shift + C: 切换元素拾取审查模式
    if (ctrl && shift && (vkCode == 'C' || vkCode == 'c')) {
        ToggleInspectMode();
        return true;
    }

    // 4. Ctrl + Shift + P: 切换性能与帧率仪表盘
    if (ctrl && shift && (vkCode == 'P' || vkCode == 'p')) {
        TogglePerformanceOverlay();
        return true;
    }

    // 5. Ctrl + Shift + D: 切换重绘脏区热力图
    if (ctrl && shift && (vkCode == 'D' || vkCode == 'd')) {
        ToggleOverdrawVisualizer();
        return true;
    }

    return false;
}

bool DevToolsManager::OnMouseMove(Point pt, UIElement* root) {
    if (m_inspectorVisible && m_inspector.ContainsPoint(pt)) {
        return m_inspector.OnMouseMove(pt);
    }

    if (m_inspectMode && root) {
        UIElement* hit = root->HitTest(pt.x, pt.y);
        if (hit != m_hoveredInspectElement) {
            m_hoveredInspectElement = hit;
            if (Window::Current()) {
                Window::Current()->InvalidateLogicalRect(Rect(0, 0, 10000, 10000));
            }
        }
        return true;
    }

    return false;
}

bool DevToolsManager::OnMouseDown(Point pt, UIElement* root) {
    if (m_inspectorVisible && m_inspector.ContainsPoint(pt)) {
        return m_inspector.OnMouseDown(pt);
    }

    if (m_inspectMode && root) {
        UIElement* hit = root->HitTest(pt.x, pt.y);
        if (hit) {
            SelectElement(hit, true);
            SetInspectMode(false);
            SetInspectorVisible(true);
        }
        return true;
    }

    return false;
}

bool DevToolsManager::OnMouseUp(Point pt) {
    if (m_inspectorVisible) {
        return m_inspector.OnMouseUp(pt);
    }
    return false;
}

bool DevToolsManager::OnMouseWheel(Point pt, float delta) {
    if (m_inspectorVisible && m_inspector.ContainsPoint(pt)) {
        return m_inspector.OnMouseWheel(pt, delta);
    }
    return false;
}

void DevToolsManager::RecordFrame(float frameTimeMs, float layoutMs, float renderMs,
                                  uint32_t drawOps, uint32_t batchReductions, uint32_t layerCacheHits) {
    m_perfOverlay.RecordFrame(frameTimeMs, layoutMs, renderMs, drawOps, batchReductions, layerCacheHits);
}

void DevToolsManager::RecordDirtyRects(const std::vector<Rect>& rects) {
    if (m_overdrawVisualizerEnabled) {
        m_overdrawVisualizer.RecordDirtyRects(rects);
    }
}

void DevToolsManager::Render(GraphicsContext& ctx, UIElement* root, Size windowSize, const std::vector<Rect>& dirtyRects) {
    // 1. 记录与绘制 Overdraw 重绘热力图
    if (m_overdrawVisualizerEnabled) {
        m_overdrawVisualizer.RecordDirtyRects(dirtyRects);
        m_overdrawVisualizer.Render(ctx, windowSize);
    }

    // 2. 盒模型诊断高亮
    const UIElement* targetToHighlight = nullptr;
    if (m_inspectMode && m_hoveredInspectElement) {
        targetToHighlight = m_hoveredInspectElement;
    } else if (m_boxModelOverlayEnabled && m_inspectorVisible) {
        targetToHighlight = m_inspector.GetSelectedElement();
    }

    if (targetToHighlight) {
        BoxModelOverlay::RenderOverlay(ctx, targetToHighlight, windowSize);
    }

    // 3. 悬浮性能仪表盘
    if (m_performanceOverlayEnabled) {
        m_perfOverlay.Render(ctx, windowSize);
    }

    // 4. DevTools 检查主面板
    if (m_inspectorVisible) {
        m_inspector.Render(ctx, windowSize);
    }
}

} // namespace CUI
