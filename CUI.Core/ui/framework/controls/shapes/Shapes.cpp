#include "Shapes.h"
#include <cmath>

namespace CUI {

// ==================== Rectangle (矩形/圆角矩形) ====================

/**
 * @brief 绘制矩形或圆角矩形的 Direct2D 渲染实现。
 * 根据 Fill 透明度决定是否填充背景，根据 Stroke 透明度和 StrokeThickness 决定是否绘制描边。
 */
void Rectangle::OnRender(GraphicsContext& ctx) {
    // 渲染采用世界坐标：元素已通过 m_bounds 定位，形状必须按绝对坐标绘制。
    Rect bounds{ m_bounds.x, m_bounds.y, m_bounds.width, m_bounds.height };
    if (bounds.width <= 0 || bounds.height <= 0) return;

    // 1. 填充内部背景
    if (m_fill.a > 0.0f) {
        if (m_cornerRadius > 0.0f) {
            ctx.FillRoundedRect(bounds, m_cornerRadius, m_fill);
        } else {
            ctx.FillRect(bounds, m_fill);
        }
    }

    // 2. 绘制外边界描边
    if (m_stroke.a > 0.0f && m_strokeThickness > 0.0f) {
        if (m_cornerRadius > 0.0f) {
            ctx.DrawRoundedRect(bounds, m_cornerRadius, m_stroke, m_strokeThickness);
        } else {
            ctx.DrawRect(bounds, m_stroke, m_strokeThickness);
        }
    }
}

// ==================== Ellipse (椭圆/圆形) ====================

/**
 * @brief 绘制椭圆或圆形的 Direct2D 渲染实现。
 * 使用 PushEllipseClip 进行椭圆区域裁剪与平滑 Fill，并使用 DrawSmoothArc 绘制抗锯齿边线。
 */
void Ellipse::OnRender(GraphicsContext& ctx) {
    float w = GetBounds().width;
    float h = GetBounds().height;
    if (w <= 0 || h <= 0) return;

    // 渲染采用世界坐标：圆心需叠加元素左上角偏移。
    Point center{ m_bounds.x + w * 0.5f, m_bounds.y + h * 0.5f };
    float radiusX = w * 0.5f;
    float radiusY = h * 0.5f;

    // 1. 填充椭圆内部
    if (m_fill.a > 0.0f) {
        ctx.PushEllipseClip(center, radiusX, radiusY);
        ctx.FillRect(Rect{ m_bounds.x, m_bounds.y, w, h }, m_fill);
        ctx.PopClip();
    }

    // 2. 绘制椭圆描边
    if (m_stroke.a > 0.0f && m_strokeThickness > 0.0f) {
        const float radius = (radiusX + radiusY) * 0.5f;
        ctx.DrawSmoothArc(center, radius, 0.0f, 6.28318530718f, m_stroke, m_strokeThickness);
    }
}

// ==================== Line (直线) ====================

/**
 * @brief 构造函数，初始化直线的起点与终点。
 */
Line::Line(float x1, float y1, float x2, float y2)
    : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2) {}

/**
 * @brief 绘制直线段的 Direct2D 渲染实现。
 * 调用 DrawSmoothLine 生成带圆头帽与双线性抗锯齿的平滑直线。
 */
void Line::OnRender(GraphicsContext& ctx) {
    D2D1_COLOR_F color = (m_stroke.a > 0.0f) ? m_stroke : m_fill;
    if (color.a <= 0.0f) return;

    // 渲染采用世界坐标：x1/y1…x2/y2 为元素局部坐标，需叠加元素左上角偏移。
    ctx.DrawSmoothLine(
        Point{ m_bounds.x + m_x1, m_bounds.y + m_y1 },
        Point{ m_bounds.x + m_x2, m_bounds.y + m_y2 },
        color, m_strokeThickness);
}

// ==================== Path (矢量路径) ====================

/**
 * @brief 构造函数，接收 SVG Path 语法字符串。
 */
Path::Path(const std::string& data) : m_data(data) {}

/**
 * @brief 渲染 SVG Path 矢量路径。
 * 自动识别 Path 字符串，将其包裹转换为完整的 Direct2D SVG Document 标记语言并高效渲染。
 */
void Path::OnRender(GraphicsContext& ctx) {
    if (m_data.empty()) return;
    // 渲染采用世界坐标：SVG 视口需叠加元素左上角偏移。
    Rect bounds{ m_bounds.x, m_bounds.y, m_bounds.width, m_bounds.height };

    // 如果 Path 包含了 SVG path 指令或 SVG 源码，直接委派 Direct2D SVG 引擎渲染
    if (GraphicsContext::LooksLikeSvg(m_data) || m_data.find('M') != std::string::npos || m_data.find('m') != std::string::npos) {
        std::string svgMarkup;
        if (m_data.find("<svg") != std::string::npos) {
            svgMarkup = m_data;
        } else {
            // 拼接成合法的 SVG path 包裹标签
            svgMarkup = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\"><path d=\"" + m_data + "\" fill=\"currentColor\"/></svg>";
        }
        ctx.DrawSvg(svgMarkup, bounds, (m_fill.a > 0.0f ? &m_fill : nullptr), GetOpacity());
    }
}

// ==================== SvgIcon (声明式 SVG DOM 元素) ====================

/**
 * @brief 构造函数，接收 SVG XML Markup 内容或文件路径。
 */
SvgIcon::SvgIcon(const std::string& source) : m_source(source) {}

/**
 * @brief 渲染声明式 SVG 节点。
 * 直接调用 Direct2D 内部的原生 SVG 引擎解析并渲染 SVG 内容，支持 TintColor 主题替换色。
 */
void SvgIcon::OnRender(GraphicsContext& ctx) {
    if (m_source.empty()) return;
    // 渲染采用世界坐标：SVG 视口需叠加元素左上角偏移。
    Rect bounds{ m_bounds.x, m_bounds.y, m_bounds.width, m_bounds.height };

    ctx.DrawSvg(m_source, bounds, (m_useTint ? &m_tintColor : nullptr), GetOpacity());
}

} // namespace CUI
