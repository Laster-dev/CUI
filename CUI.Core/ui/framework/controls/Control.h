#pragma once
#include "UIElement.h"
#include "../animation/AnimationSystem.h"

namespace CUI {

/**
 * @brief 所有具备交互特征与视觉状态呈现（例如 Hover、Pressed、Focused 等）的控件的基类。
 * 派生自 UIElement，内置了对鼠标滑过、点击、获焦等状态进行过渡动画（流畅淡入淡出）的机制。
 */
class Control : public UIElement {
public:
    Control();
    virtual ~Control() = default;

    virtual const char* GetClassName() const override { return "Control"; }

    // 重写绘制逻辑，处理 Hover/Pressed/Focused 的视觉渲染
    virtual void OnRender(GraphicsContext& ctx) override;
    
    // 更新内置淡入淡出动画的步进
    virtual bool OnAnimationTick() override;
    
    // 指明控件当前是否在进行状态切换的视觉过渡动画
    virtual bool HasSelfAnimation() const override;

    // 各种交互状态改变的回调接口，用于自适应驱动 VisualState 动画
    virtual void OnMouseEnter() override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnFocus() override;
    virtual void OnBlur() override;

protected:
    /**
     * @brief 获取融合了当前 Hover/Pressed 状态过渡比例的动画背景色。
     * @param fallback 若未设置任何自定义背景，返回的后备颜色。
     */
    D2D1_COLOR_F GetAnimatedBackground(D2D1_COLOR_F fallback);
    
    // 线性插值混合两个颜色（包含 Alpha 通道）
    D2D1_COLOR_F BlendColor(D2D1_COLOR_F from, D2D1_COLOR_F to, float t) const;
    
    // 触发并更新目标视觉过渡状态的数值（0.0f 为静止状态，1.0f 为交互状态如 Hover/Press）
    void UpdateVisualStateTarget();
    float GetVisualStateTarget() const;
    
    // 检测在当前自定义配色下，Hover 或 Pressed 视觉是否应与静止状态有所差异（用于优化是否进行过渡渲染）
    bool VisualStateChromeDiffers() const;

    AnimatedScalar m_visualStateAnim{}; ///< 视觉过渡动画当前值
    float m_visualStateTarget = 0.0f;    ///< 视觉过渡动画目标值
};

} // namespace CUI
