#pragma once
#include "Control.h"

namespace CUI {

/**
 * @brief 滑块控件。
 * 允许用户通过拖动滑块（Thumb）或按方向键，在最小值与最大值范围区间内选择一个浮点数值。
 */
class Slider : public Control {
public:
    Slider();
    virtual ~Slider() = default;

    virtual const char* GetClassName() const override { return "Slider"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获取悬浮交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 测算滑轨和手柄的默认自适应尺寸
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制滑动槽底色、高亮进度条、以及拖动手柄
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，捕获滑块并直接定位计算分值
    virtual void OnMouseMove(Point pt) override; // 鼠标拖拽移动，高频滑动更新分值
    virtual void OnMouseUp(Point pt) override; // 鼠标抬起释放拖拽捕获状态
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘方向键微调分值
    virtual bool AcceptsTabFocus() const override { return true; } // 支持键盘 Tab 导航聚焦
    virtual bool OnAnimationTick() override; // 驱动拖动值到实际显示值的平滑缓冲过渡动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于显示值平滑过渡动画中

    PropertyRef<float, PropertyId::ControlValue> ValueProperty; // 滑块当前数值的响应式双向绑定属性代理

    float GetValue() const { return m_value; } // 获取滑块当前数值
    void SetValue(float val); // 设置滑块当前数值

    float GetMinimum() const { return m_minimum; } // 获取滑动范围的最小值
    void SetMinimum(float minVal) { // 设置滑动范围的最小值
        m_minimum = minVal;
        NotifyFieldChanged(PropertyId::Minimum, Value(minVal));
    }

    float GetMaximum() const { return m_maximum; } // 获取滑动范围的最大值
    void SetMaximum(float maxVal) { // 设置滑动范围的最大值
        m_maximum = maxVal;
        NotifyFieldChanged(PropertyId::Maximum, Value(maxVal));
    }

    float GetStep() const { return m_step; } // 获取滑动微调精度步长
    void SetStep(float s) { // 设置滑动微调精度步长
        m_step = s;
        NotifyFieldChanged(PropertyId::Step, Value(s));
    }

    Event<Slider*, float>& OnValueChanged() { return m_onValueChangedEvent; } // 滑动数值更改时的事件发布中心

private:
    void UpdateValueFromPoint(Point pt); // 根据点坐标在滑轨内的比例反算实际的浮点分值
    Rect GetThumbRect() const; // 计算当前分值下手柄纽扣的局部绘制包络矩形
    Rect GetTrackRect() const; // 计算滑轨在局部坐标系下的外边界矩形
    void MarkSliderVisualDirty(const Rect& previousThumb, float previousDisplayValue); // 失效重绘滑块及指示器数值变化前后的脏区域

    float m_value = 0.0f;               // 内部缓存的实际滑块数值
    float m_minimum = 0.0f;             // 允许调节的下限极小值
    float m_maximum = 100.0f;           // 允许调节的上限极大值
    float m_step = 1.0f;                // 调节精度的递增/递减步长
    bool m_isDragging = false;          // 标识用户是否正用鼠标按住滑块进行拖拽操作
    AnimatedScalar m_displayValueAnim{}; // 拖动时或键盘操控数值变化，用于平滑更新的过渡动画
    Event<Slider*, float> m_onValueChangedEvent; // 数值更改事件对象
};

} // namespace CUI
