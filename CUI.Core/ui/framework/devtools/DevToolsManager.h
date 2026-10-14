#pragma once

#include "DevToolsTypes.h"
#include "BoxModelOverlay.h"
#include "PerformanceOverlay.h"
#include "OverdrawVisualizer.h"
#include "VisualTreeInspector.h"

namespace CUI {

/**
 * @brief 现代开发者工具与运行时诊断总管理器单例类 (In-App DevTools & Inspector Manager)
 */
class DevToolsManager {
public:
    static DevToolsManager& Instance();

    /**
     * @brief DevTools 浮动检查窗口显隐
     */
    bool IsInspectorVisible() const { return m_inspectorVisible; }
    void SetInspectorVisible(bool visible);
    void ToggleDevTools();

    /**
     * @brief 鼠标悬停元素审查模式 (Inspect Element Mode)
     */
    bool IsInspectMode() const { return m_inspectMode; }
    void SetInspectMode(bool active);
    void ToggleInspectMode();

    /**
     * @brief 盒模型诊断遮罩开关
     */
    bool IsBoxModelOverlayEnabled() const { return m_boxModelOverlayEnabled; }
    void SetBoxModelOverlayEnabled(bool enabled) { m_boxModelOverlayEnabled = enabled; }
    void ToggleBoxModelOverlay() { m_boxModelOverlayEnabled = !m_boxModelOverlayEnabled; }

    /**
     * @brief 性能与帧率仪表盘开关
     */
    bool IsPerformanceOverlayEnabled() const { return m_performanceOverlayEnabled; }
    void SetPerformanceOverlayEnabled(bool enabled) { m_performanceOverlayEnabled = enabled; }
    void TogglePerformanceOverlay() { m_performanceOverlayEnabled = !m_performanceOverlayEnabled; }

    /**
     * @brief 重绘脏区热力图开关
     */
    bool IsOverdrawVisualizerEnabled() const { return m_overdrawVisualizerEnabled; }
    void SetOverdrawVisualizerEnabled(bool enabled);
    void ToggleOverdrawVisualizer();

    /**
     * @brief 获取与设置当前选中的目标控件
     */
    UIElement* GetSelectedElement() const { return m_inspector.GetSelectedElement(); }
    void SelectElement(UIElement* element, bool forceCenter = false);

    /**
     * @brief 获取在审查模式下鼠标当前悬停的控件
     */
    UIElement* GetHoveredInspectElement() const { return m_hoveredInspectElement; }

    /**
     * @brief 键盘快捷键拦截处理 (F12, Ctrl+Shift+C, Ctrl+Shift+P, Ctrl+Shift+D 等)
     */
    bool OnKeyDown(int vkCode, bool ctrl, bool shift, bool alt);

    /**
     * @brief 鼠标指针移动处理
     */
    bool OnMouseMove(Point pt, UIElement* root);

    /**
     * @brief 鼠标左键按下处理
     */
    bool OnMouseDown(Point pt, UIElement* root);

    /**
     * @brief 鼠标左键释放处理
     */
    bool OnMouseUp(Point pt);

    /**
     * @brief 滚轮滚动处理
     */
    bool OnMouseWheel(Point pt, float delta);

    /**
     * @brief 采样并记录一帧的性能指标
     */
    void RecordFrame(float frameTimeMs, float layoutMs, float renderMs,
                     uint32_t drawOps = 0, uint32_t batchReductions = 0, uint32_t layerCacheHits = 0);

    /**
     * @brief 记录本帧发生重绘的脏矩形范围
     */
    void RecordDirtyRects(const std::vector<Rect>& rects);

    /**
     * @brief DevTools 总渲染入口 (绘制在所有场景与普通 Overlay 之上)
     */
    void Render(GraphicsContext& ctx, UIElement* root, Size windowSize, const std::vector<Rect>& dirtyRects);

    /**
     * @brief 获取可视树检查器组件
     */
    VisualTreeInspector& GetInspector() { return m_inspector; }

    /**
     * @brief 获取性能仪表盘组件
     */
    PerformanceOverlay& GetPerformanceOverlay() { return m_perfOverlay; }

    /**
     * @brief 获取重绘可视化组件
     */
    OverdrawVisualizer& GetOverdrawVisualizer() { return m_overdrawVisualizer; }

private:
    DevToolsManager();
    ~DevToolsManager() = default;

    bool m_inspectorVisible = false;
    bool m_inspectMode = false;
    bool m_boxModelOverlayEnabled = true;
    bool m_performanceOverlayEnabled = false;
    bool m_overdrawVisualizerEnabled = false;

    UIElement* m_hoveredInspectElement = nullptr;

    PerformanceOverlay m_perfOverlay;
    OverdrawVisualizer m_overdrawVisualizer;
    VisualTreeInspector m_inspector;
};

} // namespace CUI
