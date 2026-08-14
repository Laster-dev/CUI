#pragma once

#include "Control.h"
#include "../animation/AnimationSystem.h"
#include "../window/PopupHost.h"
#include <memory>
#include <string>

namespace CUI {

/**
 * @brief 浮出弹出框的默认停靠方位。
 * Top: 目标元素正上方。
 * Bottom: 目标元素正下方。
 * Left: 目标元素正左侧。
 * Right: 目标元素正右侧。
 */
enum class FlyoutPlacement {
    Top,    // 位于宿主上方停靠
    Bottom, // 位于宿主下方停靠
    Left,   // 位于宿主左侧停靠
    Right   // 位于宿主右侧停靠
};

/**
 * @brief 浮出提示卡片的内容承载容器（FlyoutPresenter）。
 * 负责包裹在具体注入的内容（Content）之外，绘制磨砂后置背景阴影与白色半透底板框。
 */
class FlyoutPresenter : public Control {
public:
    FlyoutPresenter();
    virtual ~FlyoutPresenter() = default;

    virtual const char* GetClassName() const override { return "FlyoutPresenter"; } // 获取类名

    void SetContent(std::shared_ptr<UIElement> content); // 放入并设置卡片要具体展示的子控件
    std::shared_ptr<UIElement> GetContent() const { return m_content; } // 获取所承载的子控件

    virtual Size Measure(Size availableSize) override; // 计算卡片在保留内边距和圆角下的理想尺寸大小
    virtual void Arrange(Rect finalRect) override; // 排版对齐子元素内容
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制卡片框的背景、圆角及描边线圈

private:
    std::shared_ptr<UIElement> m_content; // 被呈现的具体内容元素
};

/**
 * @brief 浮出提示框（Flyout）控件。
 * 类似于一个无需阻塞界面的小卡片气泡（Flyout），支持停靠在任意其他 UIElement 旁，点击背景空白会自动消退。实现了 IPopup 接口。
 */
class Flyout : public UIElement, public IPopup {
public:
    Flyout();
    explicit Flyout(std::shared_ptr<UIElement> content);
    virtual ~Flyout() = default;

    virtual const char* GetClassName() const override { return "Flyout"; } // 获取类名

    void SetContent(std::shared_ptr<UIElement> content); // 设置弹出框中要呈现的元素
    std::shared_ptr<UIElement> GetContent() const { return m_presenter ? m_presenter->GetContent() : nullptr; } // 获取弹出框中呈现的元素

    void SetPlacement(FlyoutPlacement placement) { m_placement = placement; } // 更改首选的停靠方位
    FlyoutPlacement GetPlacement() const { return m_placement; } // 获取首选的停靠方位

    void ShowAt(UIElement* target); // 在指定其他控件节点的边侧弹开浮出框
    void ShowAt(Point pt); // 在视口的某一个绝对二维坐标点位置弹开浮出框
    void Hide(); // 隐藏并关闭浮出框
    bool IsOpen() const { return m_isOpen; } // 获取当前浮出框是否已开启显示

    // IPopup 弹出层接口实现
    virtual bool IsPopupOpen() const override { return m_isOpen; } // 获取弹出层是否已展开
    virtual Rect GetPopupBounds() const override; // 获取浮出框在全局视口坐标系下的边界盒矩形
    virtual bool HitDismissExempt(float x, float y) const override; // 判定该点击位置是否免于强制消退收起
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); } // 弹窗穿透命中定位
    virtual void RenderPopup(GraphicsContext& ctx) override; // 绘制浮出卡片Presenter及阴影
    virtual void OnLightDismiss() override { Hide(); } // 点击背景空白消退收拢关闭

    virtual Size Measure(Size availableSize) override; // 测量大小 (Popup 为 Overlay，自身流排版测量为 0)
    virtual void Arrange(Rect finalRect) override; // 编排位置
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 实质渲染浮空卡片
    virtual UIElement* HitTestOverlay(float x, float y) override; // 弹出层碰撞测试定位
    virtual bool OnAnimationTick() override; // 驱动浮出卡片淡入及缩放弹出的平滑进入动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于进入/淡出过渡动画中
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 收集本帧动画产生的脏矩形区域
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 递归收集包含卡片子级在内的重绘脏区域
    virtual bool ShouldClipToBounds() const override { return false; } // 绝对不剪裁超出范围的弹出物

private:
    std::shared_ptr<FlyoutPresenter> m_presenter; // 内容承载器实例指针
    FlyoutPlacement m_placement = FlyoutPlacement::Bottom; // 预设的停靠放置方位
    bool m_isOpen = false;                       // 浮出框当前是否展开
    UIElement* m_anchor = nullptr;                // 绑定的相对定位目标控件弱引用指针
    Point m_popupPos{ 0.0f, 0.0f };               // 弹出框在视口的绝对定位 X、Y 坐标
    Size m_popupSize{ 220.0f, 140.0f };           // 弹出框的尺寸大小 (W, H)
    AnimatedScalar m_popupAnim{ 0.0f };           // 弹窗淡入及位移动效
};

} // namespace CUI
