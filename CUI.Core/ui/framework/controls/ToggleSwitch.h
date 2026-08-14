#pragma once
#include "Control.h"

namespace CUI {

/**
 * @brief 物理开关（ToggleSwitch）控件。
 * 类似于手机系统设置中的开/关切换钮。包含滑道背景颜色渐变、滑动纽扣弹性阻尼位移等动效。
 */
class ToggleSwitch : public Control {
public:
    ToggleSwitch();
    virtual ~ToggleSwitch() = default;

    virtual const char* GetClassName() const override { return "ToggleSwitch"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获取悬浮交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 计算开关加上右侧说明标签文本的总大小
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制开关滑道背景及指示纽扣
    virtual void OnMouseUp(Point pt) override; // 鼠标松开，切换开闭状态并派发事件
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘空格与回车以快速切换状态
    virtual bool AcceptsTabFocus() const override { return true; } // 支持键盘 Tab 导航聚焦
    virtual void OnFocus() override; // 聚焦时，标记重新绘制以渲染虚线框
    virtual void OnBlur() override; // 失去焦点时，取消绘制虚线框
    virtual bool OnAnimationTick() override; // 驱动纽扣在开关两极间的弹性过渡位移动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于位移动画中

    PropertyRef<bool, PropertyId::IsOn> IsOn; // 开关是否开启的双向绑定属性代理

    bool GetIsOn() const { return m_isOn; } // 获取开关状态
    void SetIsOn(bool on); // 设置开关状态

    const std::string& GetHeader() const { return m_header; } // 获取轨道右侧伴随的说明标签文本
    void SetHeader(const std::string& header) { // 设置轨道右侧伴随的说明标签文本
        if (m_header == header) return;
        m_header = header;
        NotifyFieldChanged(PropertyId::Header, Value(header));
        MarkRenderContentDirty();
    }

    Event<ToggleSwitch*, bool>& OnToggled() { return m_onToggledEvent; } // 状态发生改变时的事件派发中心

private:
    bool m_isOn = false;                       // 开关物理状态（true为开启，false为关闭）
    std::string m_header{ "开关 (ToggleSwitch)" }; // 右侧说明文本缓存
    AnimatedScalar m_knobPosAnim{};            // 滑块轨道位置过渡动画（0.0f 为 Off, 1.0f 为 On）
    Event<ToggleSwitch*, bool> m_onToggledEvent; // 状态改变事件对象
};

} // namespace CUI
