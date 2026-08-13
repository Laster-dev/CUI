#pragma once
#include "Control.h"

namespace CUI {

class Button : public Control {
public:
    Button();
    explicit Button(const std::string& text);
    virtual ~Button() = default;

    virtual const char* GetClassName() const override { return "Button"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    void SetText(const std::string& text) { UIElement::SetText(text); }
    const std::string& GetText() const { return UIElement::GetText(); }

protected:
    void BeginRipple(Point pt);
    bool TickRipple();
    void DrawRipple(GraphicsContext& ctx);
    void DrawButtonFace(GraphicsContext& ctx, D2D1_COLOR_F bg, D2D1_COLOR_F border, float borderThickness);
    void DrawButtonLabel(GraphicsContext& ctx, const Rect& textRect, DWRITE_TEXT_ALIGNMENT align);

    Point m_rippleCenter{};
    float m_rippleRadius = 0.0f;
    float m_rippleOpacity = 0.0f;
    bool m_rippleActive = false;
};

} // namespace CUI
