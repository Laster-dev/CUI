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
        ShapeFillProperty& operator=(Color fill) { if (owner) owner->ApplyFill(fill); return *this; }
        operator Color() const { return owner ? owner->GetFill() : Color{}; }
        Color Get() const { return owner ? owner->GetFill() : Color{}; }
    } Fill;

    /**
     * @brief 形状边框描边画刷属性代理，支持通过颜色或画刷直接赋值与隐式转换。
     */
    struct ShapeStrokeProperty {
        Shape* owner = nullptr;
        ShapeStrokeProperty() = default;
        explicit ShapeStrokeProperty(Shape* o) : owner(o) {}
        ShapeStrokeProperty& operator=(Color stroke) { if (owner) owner->ApplyStroke(stroke); return *this; }
        operator Color() const { return owner ? owner->GetStroke() : Color{}; }
        Color Get() const { return owner ? owner->GetStroke() : Color{}; }
    } Stroke;

    /**
     * @brief 形状边框描边线条宽度属性代理。
     */
    struct ShapeStrokeThicknessProperty {
        Shape* owner = nullptr;
        ShapeStrokeThicknessProperty() = default;
        explicit ShapeStrokeThicknessProperty(Shape* o) : owner(o) {}
        ShapeStrokeThicknessProperty& operator=(float t) { if (owner) owner->ApplyStrokeThickness(t); return *this; }
        operator float() const { return owner ? owner->GetStrokeThickness() : 1.0f; }
        float Get() const { return owner ? owner->GetStrokeThickness() : 1.0f; }
    } StrokeThickness;

    Color GetFill() const { return m_fill; }
    void ApplyFill(Color fill) { m_fill = fill; Invalidate(); }

    Color GetStroke() const { return m_stroke; }
    void ApplyStroke(Color stroke) { m_stroke = stroke; Invalidate(); }

    float GetStrokeThickness() const { return m_strokeThickness; }
    void ApplyStrokeThickness(float thickness) { m_strokeThickness = thickness; Invalidate(); }

    Stretch GetStretch() const { return m_stretch; }
    void ApplyStretch(Stretch stretch) { m_stretch = stretch; Invalidate(); }

    void Invalidate() { InvalidateMeasure(); InvalidateArrange(); }
    virtual void OnRender(GraphicsContext& ctx) override = 0;

protected:
    Color m_fill = Color::Transparent;
    Color m_stroke = Color::Transparent;
    float m_strokeThickness = 1.0f;
    Stretch m_stretch = Stretch::None;
};

} // namespace CUI
