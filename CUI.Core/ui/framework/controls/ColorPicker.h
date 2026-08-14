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

    virtual const char* GetClassName() const override { return "ColorPicker"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获取悬浮交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 测算色块按钮的尺寸
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制正常状态下的带圆角的选中颜色方块底盘与边框
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 渲染弹出的 HSV 大面板及色相滑条
    virtual UIElement* OnHitTestOverlay(float x, float y) override; // 弹出层非客户区命中穿透定位
    virtual bool NeedsOverlayHitTest() const override { return true; } // 指明弹层需要接受高频鼠标碰撞命中
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，依据命中的面板零件展开或修改对应分量值
    virtual void OnMouseMove(Point pt) override; // 鼠标移动，更新色相或饱和度明度调节值并重算颜色
    virtual void OnMouseUp(Point pt) override; // 鼠标抬起释放调节捕获
    virtual bool OnAnimationTick() override; // 驱动弹出层缩放淡入淡出过渡动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于弹出过渡动画中

    // IPopup 弹出层接口实现
    virtual bool IsPopupOpen() const override { return m_isPopupOpen; } // 获取弹出层是否已展开
    virtual Rect GetPopupBounds() const override; // 获取弹出层在全局视口下的包络矩形
    virtual bool HitDismissExempt(float x, float y) const override; // 判定该点击位置是否豁免消退收起
    virtual UIElement* HitTestPopup(float x, float y) override { return OnHitTestOverlay(x, y); } // 弹窗穿透命中定位
    virtual void RenderPopup(GraphicsContext& ctx) override; // 绘制 HSV 二维渐变板、一维彩虹色相条及调色格子
    virtual void OnLightDismiss() override { SetPopupOpen(false); } // 点击背景空白消退收拢关闭

    void SetPopupOpen(bool open); // 开启或折叠关闭调色弹出面板

    PropertyRef<Color, PropertyId::SelectedColor> SelectedColor; // 当前选中颜色的双向绑定属性代理
    
    D2D1_COLOR_F GetSelectedColor() const { return m_selectedColor; } // 获取当前选中颜色的 RGBA 浮点色彩结构
    void SetSelectedColor(D2D1_COLOR_F color); // 设置当前选中颜色并反算 H、S、V 分量值

    Event<ColorPicker*, D2D1_COLOR_F>& OnColorChanged() { return m_onColorChangedEvent; } // 颜色改变时的事件发布中心

private:
    enum class PopupPart { None, Canvas, Hue, Swatch }; // 标识面板上被鼠标抓取交互的细分子零件区域

    void MarkPopupDirty(); // 标记弹出层重绘脏标记
    Rect CanvasRect(const Rect& popRect) const; // 计算 HSV 二维饱和度-明度渐变大板的物理坐标区域
    Rect HueRect(const Rect& popRect) const; // 计算一维彩虹色相滑动条的物理坐标区域
    Rect SwatchRect(const Rect& popRect, size_t index) const; // 计算下方第 index 个预设调色格子小方块的边界
    PopupPart HitTestPopupPart(Point pt, int* swatchIndex = nullptr) const; // 分析点击点落在调色板的哪个零件位置
    bool ApplyPopupPoint(Point pt, bool allowSwatch); // 将调色坐标换算为具体的 H/S/V 分量改变

    D2D1_COLOR_F m_selectedColor{ 0, 0, 0, 1 }; // 当前选中的 RGBA 浮点色彩
    std::vector<D2D1_COLOR_F> m_swatches;     // 底部预设的快捷常用调色色块数组
    bool m_isPopupOpen = false;                 // 调色面板当前是否已弹开
    PopupPart m_dragPart = PopupPart::None;     // 当前鼠标正拽着哪部分进行色彩调节
    AnimatedScalar m_popupAnim{};               // 弹出层折展淡入动效
    float m_hue = 200.0f;                       // 当前配色的色相分量 (0.0f - 360.0f)
    float m_sat = 1.0f;                         // 当前配色的饱和度分量 (0.0f - 1.0f)
    float m_val = 0.8f;                         // 当前配色的明度分量 (0.0f - 1.0f)
    Event<ColorPicker*, D2D1_COLOR_F> m_onColorChangedEvent; // 颜色改变事件对象
};

} // namespace CUI
