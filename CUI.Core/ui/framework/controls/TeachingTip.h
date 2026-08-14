#pragma once

#include "Control.h"
#include "../animation/AnimationSystem.h"
#include "../window/PopupHost.h"
#include "../window/BubbleChrome.h"
#include <string>

namespace CUI {

/**
 * @brief 新手指引与悬浮教学提示控件（TeachingTip）。
 * 用于向用户指出新功能或进行特定步骤引导。支持带小三角箭头的气泡外观（BubbleChrome），支持模态或非模态弹出，实现 IPopup 接口。
 */
class TeachingTip : public Control, public IPopup {
public:
    TeachingTip();
    virtual ~TeachingTip();

    virtual const char* GetClassName() const override { return "TeachingTip"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值

    void SetTitle(const std::string& title); // 设置提示气泡的标题文本
    const std::string& GetTitle() const { return m_title; } // 获取标题文本
    void SetMessage(const std::string& message); // 设置提示气泡的详细说明正文文本
    const std::string& GetMessage() const { return m_message; } // 获取正文文本
    void SetActionText(const std::string& text); // 设置操作/执行动作按钮（如“我知道了”）的提示字
    const std::string& GetActionText() const { return m_actionText; } // 获取操作按钮文本
    void SetIsCloseVisible(bool visible); // 设定右上角小关闭叉叉是否显示
    bool GetIsCloseVisible() const { return m_closeVisible; } // 获取关闭按钮可见性
    void SetIsModal(bool modal); // 设定是否启用模态（如果启用，会阻断其它主视口的点击响应）
    bool GetIsModal() const { return m_isModal; } // 获得是否为模态
    void SetPreferredPlacement(BubblePlacement placement); // 更改箭头停靠定位的方位（如Auto、Left、Top等）
    BubblePlacement GetPreferredPlacement() const { return m_preferredPlacement; } // 获取停靠放置方位
    void SetMaxWidth(float width); // 限制气泡卡片允许拉伸的最大像素宽度
    float GetMaxWidth() const { return m_maxWidth; } // 获得最大像素宽度

    void ShowAround(UIElement* target); // 围绕并在指定目标控件外围弹开指引气泡
    void Close(); // 关闭气泡
    bool IsOpen() const { return m_isOpen; } // 获取当前指引气泡是否正处于展开显示中

    Event<>& OnAction() { return m_onAction; } // 操作按钮被点击触发的事件委托连接点
    Event<>& OnClosed() { return m_onClosed; } // 气泡消退关闭后的事件委托连接点

    // IPopup 弹出层接口实现
    virtual bool IsPopupOpen() const override { return m_isOpen; } // 获取弹出层是否已展开
    virtual Rect GetPopupBounds() const override; // 获取提示气泡在全局视口下的物理包络盒矩形
    virtual bool HitDismissExempt(float x, float y) const override; // 判定该点击位置是否免于强制消退收起
    virtual UIElement* HitTestPopup(float x, float y) override; // 气泡内小按钮及关闭叉的命中穿透定位
    virtual void RenderPopup(GraphicsContext& ctx) override; // 绘制带指示箭头的气泡圆角底盘和各内嵌文字/按钮
    virtual void OnLightDismiss() override; // 点击背景消退时触发关闭收拢
    virtual void CollectPopupDirty(Rect& dirtyRect, bool& hasDirty) const override; // 收集提示框被重绘的脏矩形范围

    virtual Size Measure(Size availableSize) override; // 测量大小 (弹出层在主流排版中大小为 0)
    virtual void Arrange(Rect finalRect) override; // 编排定位位置
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 在 Overlay 覆盖层实际绘制气泡
    virtual void OnMouseMove(Point pt) override; // 鼠标滑动，高亮右上角小叉或操作按钮
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，记录点击的子按钮
    virtual void OnMouseUp(Point pt) override; // 鼠标抬起，触发对应的 Action 命令或 Close
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘回车执行或 Esc 键关闭
    virtual bool AcceptsTabFocus() const override { return m_isOpen; } // 在展开状态下允许 Tab 导航聚焦
    virtual bool OnAnimationTick() override; // 驱动气泡展开、折拢以及淡入淡出动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于位移/淡出过渡动画中
    virtual bool IsModalOverlayOpen() const override; // 是否阻断底层窗口输入消息响应
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 收集本帧动画脏区域
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 递归收集气泡动画脏区域
    virtual bool ShouldClipToBounds() const override { return false; } // 绝对不裁剪溢出的弹出物
    virtual void OnNavigatedFrom() override; // 被切离页面时自动强制收拢气泡

private:
    enum class Hotspot : uint8_t { None, Close, Action, Card }; // 区分气泡内发生鼠标 Hover 的零件部位

    void Relayout(); // 重新编排气泡内的标题、正文、小叉和按钮的相对排版坐标
    void DirtyPopup(); // 脏化弹出层区域以申请画面刷新
    Hotspot HitHotspot(float x, float y) const; // 碰撞测试定位坐标落在气泡的哪个零件位置上
    bool SetHotHover(Hotspot hotspot); // 登记鼠标指针当前 Hover 的气泡零件

    std::string m_title;                                // 标题文本缓存
    std::string m_message;                              // 正文详细文本缓存
    std::string m_actionText;                           // 操作按钮文字缓存
    bool m_closeVisible = true;                         // 右上角关闭小叉是否显示
    bool m_isModal = false;                             // 是否为模态指引
    bool m_isOpen = false;                              // 气泡是否正处于弹开状态
    bool m_actionPressed = false;                       // 操作按钮当前是否正被鼠标按下
    float m_maxWidth = 320.0f;                          // 指引卡片允许的最宽像素行宽
    BubblePlacement m_preferredPlacement = BubblePlacement::Auto; // 首选的三角箭头指向停靠方位
    UIElement* m_anchor = nullptr;                      // 绑定的相对定位弱引用原始指针
    BubbleLayout m_layout{};                            // 缓存的带指示箭头的气泡卡片物理排版尺寸
    Rect m_titleRect{};                                 // 标题文字在气泡内的局部绘制矩形
    Rect m_bodyRect{};                                  // 正文说明在气泡内的局部绘制矩形
    Rect m_closeRect{};                                 // 右上角小关闭叉在气泡内的局部绘制矩形
    Rect m_actionRect{};                                // 右下角操作按钮在气泡内的局部绘制矩形
    Hotspot m_hotHover = Hotspot::None;                 // 鼠标当前悬停的气泡零件位置
    AnimatedScalar m_popupAnim{ 0.0f };                 // 气泡进入/淡出及箭定位平移过渡动画
    Event<> m_onAction;                                 // Action 事件分发对象
    Event<> m_onClosed;                                 // Close 事件分发对象
};

} // namespace CUI
