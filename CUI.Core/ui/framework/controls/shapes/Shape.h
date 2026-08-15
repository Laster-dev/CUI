#pragma once
#include "../UIElement.h"
#include "../Image.h" // 引用 Stretch 枚举类型

namespace CUI {

/**
 * @brief 所有 2D 声明式矢量图形 DOM 节点的抽象基类（Shape）。
 */
class Shape : public UIElement {
public:
    Shape();
    virtual ~Shape() = default;

    virtual const char* GetClassName() const override { return "Shape"; }

    /**
     * @brief 形状填充画刷属性代理，支持通过颜色或画刷直接赋值与隐式转换。
     */
    struct ShapeFillProperty {
        Shape* owner = nullptr;
        ShapeFillProperty() = default;
        explicit ShapeFillProperty(Shape* o) : owner(o) {}
        ShapeFillProperty& operator=(D2D1_COLOR_F fill) { if (owner) owner->SetFill(fill); return *this; }
        operator D2D1_COLOR_F() const { return owner ? owner->GetFill() : D2D1_COLOR_F{}; }
        D2D1_COLOR_F Get() const { return owner ? owner->GetFill() : D2D1_COLOR_F{}; }
    } Fill;

    /**
     * @brief 形状边框描边画刷属性代理，支持通过颜色或画刷直接赋值与隐式转换。
     */
    struct ShapeStrokeProperty {
        Shape* owner = nullptr;
        ShapeStrokeProperty() = default;
        explicit ShapeStrokeProperty(Shape* o) : owner(o) {}
        ShapeStrokeProperty& operator=(D2D1_COLOR_F stroke) { if (owner) owner->SetStroke(stroke); return *this; }
        operator D2D1_COLOR_F() const { return owner ? owner->GetStroke() : D2D1_COLOR_F{}; }
        D2D1_COLOR_F Get() const { return owner ? owner->GetStroke() : D2D1_COLOR_F{}; }
    } Stroke;

    /**
     * @brief 形状边框描边线条宽度属性代理。
     */
    struct ShapeStrokeThicknessProperty {
        Shape* owner = nullptr;
        ShapeStrokeThicknessProperty() = default;
        explicit ShapeStrokeThicknessProperty(Shape* o) : owner(o) {}
        ShapeStrokeThicknessProperty& operator=(float t) { if (owner) owner->SetStrokeThickness(t); return *this; }
        operator float() const { return owner ? owner->GetStrokeThickness() : 1.0f; }
        float Get() const { return owner ? owner->GetStrokeThickness() : 1.0f; }
    } StrokeThickness;

    D2D1_COLOR_F GetFill() const { return m_fill; }
    void SetFill(D2D1_COLOR_F fill) { m_fill = fill; Invalidate(); }

    D2D1_COLOR_F GetStroke() const { return m_stroke; }
    void SetStroke(D2D1_COLOR_F stroke) { m_stroke = stroke; Invalidate(); }

    float GetStrokeThickness() const { return m_strokeThickness; }
    void SetStrokeThickness(float thickness) { m_strokeThickness = thickness; Invalidate(); }

    Stretch GetStretch() const { return m_stretch; }
    void SetStretch(Stretch stretch) { m_stretch = stretch; Invalidate(); }

    void Invalidate() { InvalidateMeasure(); InvalidateArrange(); }
    virtual void OnRender(GraphicsContext& ctx) override = 0;

protected:
    D2D1_COLOR_F m_fill = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);
    D2D1_COLOR_F m_stroke = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);
    float m_strokeThickness = 1.0f;
    Stretch m_stretch = Stretch::None;
};

} // namespace CUI
