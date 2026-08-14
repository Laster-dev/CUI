#pragma once

#include "Control.h"
#include "MenuBar.h"
#include "../animation/AnimationSystem.h"
#include "../window/IWindowChrome.h"

#include <wrl/client.h>
#include <d2d1_1.h>

namespace CUI {

/**
 * @brief 窗口标题栏控件。
 * 承载窗口的标题、小图标、菜单栏（MenuBar）、以及右上角的最小化、最大化/还原、关闭原生按钮（Caption Buttons）。
 * 实现了 IWindowChrome 接口，托管 Win32 非客户区命中测试与拖拽拉伸。
 */
class WindowTitleBar : public Control, public IWindowChrome {
public:
    WindowTitleBar();
    virtual ~WindowTitleBar();

    virtual const char* GetClassName() const override { return "WindowTitleBar"; } // 获取类名
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制标题栏静态底盘、文字和边框
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 绘制右上角悬浮的最小化/最大化/关闭按钮状态与过渡色
    virtual void Arrange(Rect finalRect) override; // 编排排列子控件布局
    virtual void OnThemeChanged() override; // 响应主题配色更改
    virtual void OnMouseMove(Point pt) override; // 鼠标滑动事件入口
    virtual void OnMouseLeave() override; // 鼠标离开标题栏区域
    virtual bool OnAnimationTick() override; // 驱动按钮悬停动画时钟
    virtual bool HasSelfAnimation() const override; // 检查是否存在未完成的按钮过渡动画
    virtual void ResetMenuInteraction() override; // 充值并重置菜单栏交互激活状态

    virtual UIElement* GetChromeElement() override { return this; } // 获取所承载的视觉元素本身强转指针
    virtual const UIElement* GetChromeElement() const override { return this; }
    virtual bool IsInteractiveHit(float x, float y) const override; // 检查指定坐标点是否落在可接受点击交互的区域内（例如菜单栏、右侧自定义区）
    virtual bool IsCaptionDragHit(float x, float y, UIElement* treeHit) const override; // 检查坐标是否落在允许拖动窗口移动的标题栏空白抓取区
    virtual LRESULT HitTestNonClient(float x, float y) const override; // 处理 Win32 WM_NCHITTEST 非客户区碰撞测试
    virtual bool ConsumeChromeDirty() override; // 提取并擦除标题栏非客户区脏重绘标记
    virtual void NotifyNonClientMouseMove(float x, float y) override; // 响应并同步 Win32 非客户区鼠标移动事件
    virtual void NotifyNonClientMouseLeave() override; // 响应 Win32 非客户区鼠标离去事件

    MenuBar& GetMenuBar() { return *m_menuBar; } // 获取内部持有的菜单栏引用
    const MenuBar& GetMenuBar() const { return *m_menuBar; }

    void SetRightContent(const std::shared_ptr<UIElement>& content); // 设置在标题栏右侧（窗口按钮左侧）自定义注入填充的视觉子元素
    std::shared_ptr<UIElement> GetRightContent() const { return m_rightContent; } // 获取右侧自定义注入填充的视觉子元素

    void SetTitle(const std::string& title); // 设置标题栏主文本
    const std::string& GetTitle() const { return m_title; } // 获取标题栏主文本

    void SetIconText(const std::string& iconText); // 设置以文本符号形式表达的左侧小图标标签
    const std::string& GetIconText() const { return m_iconText; }

    void SetNativeIcon(HICON icon, bool takeOwnership = false); // 设置原生 Windows 操作系统 HICON 格式文件图标
    HICON GetNativeIcon() const { return m_nativeIcon; }

    Rect GetMinimizeButtonRect() const; // 计算并返回最小化按钮在标题栏局部坐标系下的矩形包络盒
    Rect GetMaximizeButtonRect() const; // 计算并返回最大化按钮在标题栏局部坐标系下的矩形包络盒
    Rect GetCloseButtonRect() const;    // 计算并返回关闭按钮在标题栏局部坐标系下的矩形包络盒

private:
    bool IsMenuBarHit(float x, float y) const; // 坐标点是否落入菜单栏区域
    bool IsRightContentHit(float x, float y) const; // 坐标点是否落入右侧自定义元素区
    bool IsCaptionButtonHit(float x, float y) const; // 坐标点是否落入窗口三大控制按钮（最小化/最大化/关闭）中
    int HitTestHoverRegion(float x, float y) const; // 判断鼠标落入哪个具体的控制按钮区域（返回区域编号索引）
    Rect LayoutMenuBar(GraphicsContext& ctx); // 对菜单栏进行排版计算并定位
    Rect LayoutRightContent(); // 对右侧自定义元素进行排版定位
    bool EnsureNativeIconBitmap(GraphicsContext& ctx); // 确保将原生 HICON 句柄转化为 Direct2D 能够渲染的位图纹理
    void ApplyHoverRegion(int region, bool forceDirty); // 更新并激活按钮悬停区域，标记画面变脏
    void SyncCaptionHoverTargets(); // 同步三大按钮的目标过渡淡入淡出动画参数

    std::shared_ptr<MenuBar> m_menuBar;                               // 菜单栏控件实例共享引用
    std::shared_ptr<UIElement> m_rightContent;                        // 右侧自定义注入的元素实例引用
    std::string m_title;                                              // 标题栏主标题文本
    std::string m_iconText;                                           // 以文本符号表达的图标标签
    HICON m_nativeIcon = nullptr;                                     // 原生 Windows 系统图标 HICON 句柄
    bool m_ownsNativeIcon = false;                                    // 指示当前标题栏是否独占该 HICON 句柄并在析构时负责销毁它
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_nativeIconBitmap;           // 转换后的 D2D 渲染位图缓存对象
    bool m_menuChromeDirty = false;                                   // 标记标题栏局部需要重绘的非客户区脏标记
    int m_hoverRegion = -1;                                           // 鼠标当前悬浮在哪个按钮区域编号（-1为无，1为最小化，2为最大化，3为关闭）
    AnimatedScalar m_minHoverAnim{ 0.0f };                            // 最小化按钮 Hover 状态淡入淡出动画时钟
    AnimatedScalar m_maxHoverAnim{ 0.0f };                            // 最大化按钮 Hover 状态淡入淡出动画时钟
    AnimatedScalar m_closeHoverAnim{ 0.0f };                          // 关闭按钮 Hover 状态淡入淡出动画时钟
};

} // namespace CUI
