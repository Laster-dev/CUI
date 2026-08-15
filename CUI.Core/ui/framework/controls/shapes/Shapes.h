#pragma once
#include "Shape.h"
#include <string>

namespace CUI {

/**
 * @brief 矩形与圆角矩形 声明式 Shape 元素 (Rectangle)。
 * 
 * 在 UI 树中作为一个标准的 UIElement 节点渲染直角矩形或圆角矩形，
 * 支持独立的 Fill 填充色、Stroke 描边色以及 CornerRadius 圆角半径设置。
 */
class Rectangle : public Shape {
public:
    Rectangle() = default;
    virtual ~Rectangle() = default;

    /**
     * @brief 获取类名标识。
     * @return 字符串指针 "Rectangle"。
     */
    virtual const char* GetClassName() const override { return "Rectangle"; }

    /**
     * @brief 获取矩形四个角的圆角半径（像素）。
     * @return 圆角半径。
     */
    float GetCornerRadius() const { return m_cornerRadius; }

    /**
     * @brief 设置矩形四个角的圆角半径（像素），并重绘。
     * @param radius 圆角半径值。
     */
    void SetCornerRadius(float radius) { m_cornerRadius = radius; Invalidate(); }

    /**
     * @brief 渲染矩形/圆角矩形几何图形。
     * @param ctx GraphicsContext 绘图上下文引用。
     */
    virtual void OnRender(GraphicsContext& ctx) override;

private:
    float m_cornerRadius = 0.0f; ///< 矩形圆角半径，默认 0.0f（直角矩形）
};

/**
 * @brief 椭圆与圆形 声明式 Shape 元素 (Ellipse)。
 * 
 * 在控件分配的包络边界内内切绘制圆或椭圆，
 * 支持渐变/单色填充与平滑抗锯齿描边。
 */
class Ellipse : public Shape {
public:
    Ellipse() = default;
    virtual ~Ellipse() = default;

    /**
     * @brief 获取类名标识。
     * @return 字符串指针 "Ellipse"。
     */
    virtual const char* GetClassName() const override { return "Ellipse"; }

    /**
     * @brief 渲染椭圆/圆形几何图形。
     * @param ctx GraphicsContext 绘图上下文引用。
     */
    virtual void OnRender(GraphicsContext& ctx) override;
};

/**
 * @brief 直线 声明式 Shape 元素 (Line)。
 * 
 * 在坐标系中从起点 (X1, Y1) 到终点 (X2, Y2) 绘制一条带有抗锯齿圆头的直线段。
 */
class Line : public Shape {
public:
    Line() = default;

    /**
     * @brief 带起始点坐标参数的构造函数。
     * @param x1 起点 X 绝对/局部像素坐标。
     * @param y1 起点 Y 绝对/局部像素坐标。
     * @param x2 终点 X 绝对/局部像素坐标。
     * @param y2 终点 Y 绝对/局部像素坐标。
     */
    Line(float x1, float y1, float x2, float y2);
    virtual ~Line() = default;

    /**
     * @brief 获取类名标识。
     * @return 字符串指针 "Line"。
     */
    virtual const char* GetClassName() const override { return "Line"; }

    /** @brief 获取起点 X 坐标。 */
    float GetX1() const { return m_x1; }
    /** @brief 设置起点 X 坐标，并重绘。 */
    void SetX1(float x) { m_x1 = x; Invalidate(); }

    /** @brief 获取起点 Y 坐标。 */
    float GetY1() const { return m_y1; }
    /** @brief 设置起点 Y 坐标，并重绘。 */
    void SetY1(float y) { m_y1 = y; Invalidate(); }

    /** @brief 获取终点 X 坐标。 */
    float GetX2() const { return m_x2; }
    /** @brief 设置终点 X 坐标，并重绘。 */
    void SetX2(float x) { m_x2 = x; Invalidate(); }

    /** @brief 获取终点 Y 坐标。 */
    float GetY2() const { return m_y2; }
    /** @brief 设置终点 Y 坐标，并重绘。 */
    void SetY2(float y) { m_y2 = y; Invalidate(); }

    /**
     * @brief 渲染平滑抗锯齿直线段。
     * @param ctx GraphicsContext 绘图上下文引用。
     */
    virtual void OnRender(GraphicsContext& ctx) override;

private:
    float m_x1 = 0.0f; ///< 起点 X 局部坐标
    float m_y1 = 0.0f; ///< 起点 Y 局部坐标
    float m_x2 = 0.0f; ///< 终点 X 局部坐标
    float m_y2 = 0.0f; ///< 终点 Y 局部坐标
};

/**
 * @brief 矢量 Path 路径声明式 Shape 元素 (Path)。
 * 
 * 支持标准的 SVG Path Data 语法（例如 "M 10 10 L 90 10 Z"）或完整的 SVG Path 字符串。
 */
class Path : public Shape {
public:
    Path() = default;

    /**
     * @brief 构造函数，传入 SVG Path 语法字符串。
     * @param data SVG path data (d 属性值)。
     */
    explicit Path(const std::string& data);
    virtual ~Path() = default;

    /**
     * @brief 获取类名标识。
     * @return 字符串指针 "Path"。
     */
    virtual const char* GetClassName() const override { return "Path"; }

    /** @brief 获取当前路径的数据字符串。 */
    std::string GetData() const { return m_data; }
    /** @brief 设置 SVG Path 数据字符串，并重绘。 */
    void SetData(const std::string& data) { m_data = data; Invalidate(); }

    /**
     * @brief 渲染 SVG 矢量路径。
     * @param ctx GraphicsContext 绘图上下文引用。
     */
    virtual void OnRender(GraphicsContext& ctx) override;

private:
    std::string m_data; ///< SVG 路径数据字符串
};

/**
 * @brief SVG 声明式 DOM 节点元素 (SvgIcon)。
 * 
 * 将完整 SVG XML 字符串 markup 或文件路径作为独立的 DOM 节点插入 UIElement 树中。
 * 特点：
 * 1. 作为 DOM 节点，可以直接绑定 OnMouseDown、OnMouseEnter 等事件处理函数。
 * 2. 支持通过 TintColor 进行主题着色/单色替换。
 */
class SvgIcon : public Shape {
public:
    SvgIcon() = default;

    /**
     * @brief 构造函数，传入 SVG XML Markup 字符串或 SVG 文件路径。
     * @param source SVG 文本内容或路径。
     */
    explicit SvgIcon(const std::string& source);
    virtual ~SvgIcon() = default;

    /**
     * @brief 获取类名标识。
     * @return 字符串指针 "SvgIcon"。
     */
    virtual const char* GetClassName() const override { return "SvgIcon"; }

    /** @brief 获取 SVG 源码或路径。 */
    std::string GetSource() const { return m_source; }
    /** @brief 设置 SVG 源码或路径，并重绘。 */
    void SetSource(const std::string& source) { m_source = source; Invalidate(); }

    /** @brief 获取着色覆盖颜色。 */
    D2D1_COLOR_F GetTintColor() const { return m_tintColor; }
    /** @brief 设置单色着色覆盖颜色 (TintColor)，用于适应主题配色。 */
    void SetTintColor(D2D1_COLOR_F color) { m_tintColor = color; m_useTint = true; Invalidate(); }
    /** @brief 清除着色覆盖，还原 SVG 本身的原始 Fill/Stroke 颜色。 */
    void ClearTintColor() { m_useTint = false; Invalidate(); }

    /**
     * @brief 调用 Direct2D 原生 SVG 引擎进行矢量绘制。
     * @param ctx GraphicsContext 绘图上下文引用。
     */
    virtual void OnRender(GraphicsContext& ctx) override;

private:
    std::string m_source;                                           ///< SVG XML 标记语言字符串或文件路径
    D2D1_COLOR_F m_tintColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f); ///< 主题着色色值
    bool m_useTint = false;                                         ///< 是否启用主题着色替换标志
};

} // namespace CUI
