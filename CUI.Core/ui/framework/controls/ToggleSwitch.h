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

    virtual const char* GetClassName() const override { return "ToggleSwitch"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual void OnFocus() override;
    virtual void OnBlur() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    PropertyRef<bool, PropertyId::IsOn> IsOn; ///< 开关是否开启的双向绑定属性

    bool GetIsOn() const { return m_isOn; }
    void SetIsOn(bool on);

    /**
     * @brief 获取并设置轨道右侧伴随的说明标签文本。
     */
    const std::string& GetHeader() const { return m_header; }
    void SetHeader(const std::string& header) {
        if (m_header == header) return;
        m_header = header;
        NotifyFieldChanged(PropertyId::Header, Value(header));
        MarkRenderContentDirty();
    }

    Event<ToggleSwitch*, bool>& OnToggled() { return m_onToggledEvent; }

private:
    bool m_isOn = false;
    std::string m_header{ "开关 (ToggleSwitch)" };
    AnimatedScalar m_knobPosAnim{}; ///< 滑块轨道位置动画（0.0f 为 Off, 1.0f 为 On）
    Event<ToggleSwitch*, bool> m_onToggledEvent;
};

} // namespace CUI
