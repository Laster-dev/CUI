#pragma once
#include "../UIElement.h"
#include "../Image.h" // 引用 Stretch 枚举类型

namespace CUI {

/**
 * @brief 所有 2D 声明式矢量图形 DOM 节点的抽象基类（Shape）。
 * 
 * Shape 继承自 UIElement，作为 UI 树（DOM 节点树）中的独立节点存在。
 * 主要特性：
 * 1. **声明式与 DOM 节点支持**：可直接插入 Canvas/Grid/StackPanel 等任意容器中，支持响应式数据绑定与 DSL 链式声明。
 * 2. **独立事件捕获**：因为属于 UIElement 节点，天然融入 CUI 的 HitTest 与事件派发系统，
 *    可直接绑定 OnMouseDown、OnMouseEnter、OnMouseLeave 等事件，无需手动转换坐标判定碰撞。
 * 3. **图形外观控制**：统一提供 Fill（填充色）、Stroke（描边色）、StrokeThickness（描边粗细）与 Stretch（拉伸规则）等属性。
 */
class Shape : public UIElement {
public:
    /**
     * @brief 构造函数，初始化 Shape 节点基类。
     */
    Shape();

    /**
     * @brief 虚析构函数。
     */
    virtual ~Shape() = default;

    /**
     * @brief 获取当前控件的类名标识。
     * @return 字符串指针 "Shape"。
     */
    virtual const char* GetClassName() const override { return "Shape"; }

    /**
     * @brief 获取图形的内部填充颜色 (Fill)。
     * @return D2D1_COLOR_F 颜色结构体。
     */
    D2D1_COLOR_F GetFill() const { return m_fill; }

    /**
     * @brief 设置图形的内部填充颜色 (Fill)，并触发重新渲染。
     * @param fill 目标填充颜色。
     */
    void SetFill(D2D1_COLOR_F fill) { m_fill = fill; Invalidate(); }

    /**
     * @brief 获取图形的外边框描边颜色 (Stroke)。
     * @return D2D1_COLOR_F 描边颜色。
     */
    D2D1_COLOR_F GetStroke() const { return m_stroke; }

    /**
     * @brief 设置图形的外边框描边颜色 (Stroke)，并触发重新渲染。
     * @param stroke 目标描边颜色。
     */
    void SetStroke(D2D1_COLOR_F stroke) { m_stroke = stroke; Invalidate(); }

    /**
     * @brief 获取图形的描边粗细像素值 (StrokeThickness)。
     * @return 描边宽度（单位：DIP/像素）。
     */
    float GetStrokeThickness() const { return m_strokeThickness; }

    /**
     * @brief 设置图形的描边粗细像素值 (StrokeThickness)，并触发重新布局与重新渲染。
     * @param thickness 描边宽度像素值。
     */
    void SetStrokeThickness(float thickness) { m_strokeThickness = thickness; Invalidate(); }

    /**
     * @brief 获取图形在父容器中的伸缩拉伸规则 (Stretch)。
     * @return Stretch 比例枚举（None, Fill, Uniform, UniformToFill）。
     */
    Stretch GetStretch() const { return m_stretch; }

    /**
     * @brief 设置图形在父容器中的伸缩拉伸规则 (Stretch)。
     * @param stretch 目标拉伸规则。
     */
    void SetStretch(Stretch stretch) { m_stretch = stretch; Invalidate(); }

    /**
     * @brief 触发当前 Shape 节点的渲染与测量失效，使其重绘。
     */
    void Invalidate() { InvalidateMeasure(); InvalidateArrange(); }

    /**
     * @brief 纯虚渲染函数，由各派生 Shape 控件（Rectangle, Ellipse, Line 等）具体实现 Direct2D 绘制逻辑。
     * @param ctx 核心 GraphicsContext 绘图上下文引用。
     */
    virtual void OnRender(GraphicsContext& ctx) override = 0;

protected:
    D2D1_COLOR_F m_fill = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);     ///< 图形内部填充颜色，默认设为完全透明
    D2D1_COLOR_F m_stroke = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);   ///< 图形外边界描边颜色，默认设为完全透明
    float m_strokeThickness = 1.0f;                                  ///< 描边边框宽度（像素），默认 1.0f
    Stretch m_stretch = Stretch::None;                              ///< 矢量图形缩放自适应规则
};

} // namespace CUI
