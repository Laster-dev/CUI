#pragma once
#include "Control.h"
#include "TextBox.h"
#include "../style/ThemeManager.h"
#include "../window/PopupHost.h"

namespace CUI {

/**
 * @brief 颜色选择器控件（ColorPicker）。
 * 类似于设计软件中的颜色面板。包含色相滑块（Hue Slider）、饱和度与明度二维画布（SV Canvas）、预设常用色板（Swatches），并提供弹出层展示。
 */
class ColorPicker : public Control, public IPopup {
public:
    ColorPicker();
    virtual ~ColorPicker() = default;

    virtual const char* GetClassName() const override { return "ColorPicker"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* OnHitTestOverlay(float x, float y) override;
    virtual bool NeedsOverlayHitTest() const override { return true; }
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    // IPopup 弹出层接口实现
    virtual bool IsPopupOpen() const override { return m_isPopupOpen; }
    virtual Rect GetPopupBounds() const override;
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return OnHitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { SetPopupOpen(false); }

    void SetPopupOpen(bool open);

    PropertyRef<Color, PropertyId::SelectedColor> SelectedColor; ///< 当前选中颜色的双向绑定属性
    
    D2D1_COLOR_F GetSelectedColor() const { return m_selectedColor; }
    void SetSelectedColor(D2D1_COLOR_F color);

    Event<ColorPicker*, D2D1_COLOR_F>& OnColorChanged() { return m_onColorChangedEvent; }

private:
    enum class PopupPart { None, Canvas, Hue, Swatch };

    void MarkPopupDirty();
    Rect CanvasRect(const Rect& popRect) const;
    Rect HueRect(const Rect& popRect) const;
    Rect SwatchRect(const Rect& popRect, size_t index) const;
    PopupPart HitTestPopupPart(Point pt, int* swatchIndex = nullptr) const;
    bool ApplyPopupPoint(Point pt, bool allowSwatch);

    D2D1_COLOR_F m_selectedColor{ 0, 0, 0, 1 };
    std::vector<D2D1_COLOR_F> m_swatches;
    bool m_isPopupOpen = false;
    PopupPart m_dragPart = PopupPart::None;
    AnimatedScalar m_popupAnim{}; ///< 弹出层展开折叠的缩放淡入淡出动画
    float m_hue = 200.0f; // 0..360 色相
    float m_sat = 1.0f;   // 0..1 饱和度
    float m_val = 0.8f;   // 0..1 明度
    Event<ColorPicker*, D2D1_COLOR_F> m_onColorChangedEvent;
};

} // namespace CUI
