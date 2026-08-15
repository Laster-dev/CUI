#pragma once
#include "Control.h"

namespace CUI {

/**
 * @brief 范围滑块。
 * 由两个滑块（Lower 与 Upper）组成，用于让用户选择一个区间段范围（如筛选价格区间）。
 */
class RangeSlider : public Control {
public:
    RangeSlider();
    virtual ~RangeSlider() = default;

    virtual const char* GetClassName() const override { return "RangeSlider"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获取悬浮交互鼠标样式
    bool AcceptsTabFocus() const override { return true; } // 支持键盘 Tab 导航聚焦

    virtual Size Measure(Size availableSize) override; // 测量区间滑动条自适应大小
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制滑动轨道、双纽扣手柄、以及中间高亮区
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，判定并捕获较近的那一个滑块手柄
    virtual void OnMouseMove(Point pt) override; // 鼠标拖动更新对应滑块的浮点分值
    virtual void OnMouseUp(Point pt) override; // 鼠标抬起释放拖拽状态
    virtual void OnMouseLeave() override; // 鼠标移开重置悬停手柄状态
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘方向键微调选中的那一个手柄的值
    virtual bool OnAnimationTick() override; // 驱动双滑块显示值平滑滑动过渡动画
    virtual bool HasSelfAnimation() const override; // 检查双滑块是否仍有动画正在播放

    struct RangeSliderMinimumProperty {
        RangeSlider* owner = nullptr;
        RangeSliderMinimumProperty() = default;
        explicit RangeSliderMinimumProperty(RangeSlider* o) : owner(o) {}
        RangeSliderMinimumProperty& operator=(float v) { if (owner) owner->SetMinimum(v); return *this; }
        operator float() const { return owner ? owner->GetMinimum() : 0.0f; }
        float Get() const { return owner ? owner->GetMinimum() : 0.0f; }
    } Minimum;

    struct RangeSliderMaximumProperty {
        RangeSlider* owner = nullptr;
        RangeSliderMaximumProperty() = default;
        explicit RangeSliderMaximumProperty(RangeSlider* o) : owner(o) {}
        RangeSliderMaximumProperty& operator=(float v) { if (owner) owner->SetMaximum(v); return *this; }
        operator float() const { return owner ? owner->GetMaximum() : 100.0f; }
        float Get() const { return owner ? owner->GetMaximum() : 100.0f; }
    } Maximum;

    struct RangeSliderStepProperty {
        RangeSlider* owner = nullptr;
        RangeSliderStepProperty() = default;
        explicit RangeSliderStepProperty(RangeSlider* o) : owner(o) {}
        RangeSliderStepProperty& operator=(float v) { if (owner) owner->SetStep(v); return *this; }
        operator float() const { return owner ? owner->GetStep() : 1.0f; }
        float Get() const { return owner ? owner->GetStep() : 1.0f; }
    } Step;

    float GetMinimum() const { return m_minimum; } // 获取滑动最小值
    void SetMinimum(float minVal); // 设置滑动最小值
    float GetMaximum() const { return m_maximum; } // 获取滑动最大值
    void SetMaximum(float maxVal); // 设置滑动最大值
    float GetStep() const { return m_step; } // 获取调节步长精度
    void SetStep(float step); // 设置调节步长精度
    
    float GetMinimumRange() const { return m_minimumRange; } // 获取允许选择的最小跨度范围大小
    void SetMinimumRange(float range); // 设置允许选择的最小跨度范围大小

    PropertyRef<float, PropertyId::LowerValue> LowerValue; // 下限滑块位置值的双向绑定属性代理
    PropertyRef<float, PropertyId::UpperValue> UpperValue; // 上限滑块位置值的双向绑定属性代理
    
    float GetLowerValue() const { return m_lower; } // 获取下限数值
    void SetLowerValue(float val); // 设置下限数值
    float GetUpperValue() const { return m_upper; } // 获取上限数值
    void SetUpperValue(float val); // 设置上限数值
    void SetRange(float lower, float upper); // 强行一次性指定区间段上下限

    Event<RangeSlider*, float, float>& OnValueChanged() { return m_onValueChanged; } // 区间值改变时的事件发布中心

private:
    enum class Thumb { None, Lower, Upper }; // 标记双滑块中的特定手柄对象

    float Snap(float val) const; // 对分值进行步长对齐与四舍五入
    float ClampLower(float val) const; // 约束并限制下限滑块的值不越界
    float ClampUpper(float val) const; // 约束并限制上限滑块的值不越界
    float ValueFromPoint(Point pt) const; // 根据点在轨道的水平位置百分比换算出实际浮点值
    Rect GetTrackRect() const; // 计算滑轨的局部边界盒
    Rect GetThumbRect(float displayValue) const; // 根据指定分值计算出其对应的纽扣手柄绘制包络盒
    Rect GetLowerThumbRect() const; // 计算当前下限滑块的局部包络矩形
    Rect GetUpperThumbRect() const; // 计算当前上限滑块的局部包络矩形
    Rect GetFillRect() const; // 计算双滑块中间高亮选通区矩形
    Thumb HitTestThumb(Point pt) const; // 检测坐标落在哪个滑块手柄上方
    Thumb CloserThumb(Point pt) const; // 在点击轨道空白处时，计算哪个滑块离点击点更近
    void MarkThumbMoved(const Rect& prevThumb, const Rect& currThumb); // 失效重绘手柄位移前后的脏区域
    void MarkBothThumbsDirty(const Rect& prevLower, const Rect& prevUpper); // 批量脏化双滑块所在的所有区域
    Rect ChipFootprint(const Rect& thumb) const; // 计算滑块上方气泡指示器（Value Chip）的渲染尺寸包络
    void FlushPropertyNotify(); // 分发 Lower/Upper 双向绑定属性变化通知
    void FireChanged(); // 派发 ValueChanged 区间改变事件
    void DrawValueChip(GraphicsContext& ctx, const Rect& thumb, float value, bool active); // 在滑块上方绘制当前分值小浮片

    float m_minimum = 0.0f;             // 允许调节的最小值
    float m_maximum = 100.0f;           // 允许调节的最大值
    float m_step = 1.0f;                // 滑动精度步长
    float m_minimumRange = 0.0f;        // 限制上下限滑块之间必须保留的最小差值跨度
    float m_lower = 20.0f;              // 下限滑块数值
    float m_upper = 80.0f;              // 上限滑块数值
    AnimatedScalar m_lowerAnim{};       // 下滑块数值平滑过渡动画
    AnimatedScalar m_upperAnim{};       // 上滑块数值平滑过渡动画
    Thumb m_active = Thumb::Lower;      // 当前处于捕获操作状态中的滑块手柄
    Thumb m_hover = Thumb::None;        // 指针滑过悬停的滑块手柄
    bool m_dragging = false;            // 标记是否处于滑块拖拽状态中
    Event<RangeSlider*, float, float> m_onValueChanged; // 区间段变化事件对象
};

} // namespace CUI
