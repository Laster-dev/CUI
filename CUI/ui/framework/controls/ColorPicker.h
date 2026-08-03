#pragma once
#include "Control.h"
#include "TextBox.h"

namespace CUI {

class ColorPicker : public Control {
public:
    ColorPicker();
    virtual ~ColorPicker() = default;

    virtual const char* GetClassName() const override { return "ColorPicker"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* OnHitTestOverlay(float x, float y) override;
    virtual void OnMouseDown(Point pt) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    bool IsPopupOpen() const { return m_isPopupOpen; }
    void SetPopupOpen(bool open) { m_isPopupOpen = open; }

    D2D1_COLOR_F GetSelectedColor() const { return GetProperty("selectedColor").AsColor(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)); }
    void SetSelectedColor(D2D1_COLOR_F color);

    Event<ColorPicker*, D2D1_COLOR_F>& OnColorChanged() { return m_onColorChangedEvent; }

private:
    std::vector<D2D1_COLOR_F> m_swatches;
    bool m_isPopupOpen = false;
    AnimatedScalar m_popupAnim{};
    float m_hue = 200.0f; // 0..360
    float m_sat = 1.0f;   // 0..1
    float m_val = 0.8f;   // 0..1
    Event<ColorPicker*, D2D1_COLOR_F> m_onColorChangedEvent;
};

} // namespace CUI
