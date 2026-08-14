#pragma once
#include "UIElement.h"
#include "Button.h"
#include "TextBlock.h"
#include "TextBox.h"
#include "../render/RenderLayer.h"
#include <functional>
#include <string>
#include <memory>
#include <chrono>

namespace CUI {

/**
 * @brief 模态对话框的操作返回值。
 * None: 无。
 * Primary: 点击主确认按钮。
 * Secondary: 点击副辅助操作按钮。
 * Cancel: 点击取消或关闭。
 */
enum class DialogResult {
    None,      // 无结果
    Primary,   // 确认操作结果
    Secondary, // 辅助操作结果
    Cancel     // 取消操作结果
};

/**
 * @brief 内容对话框（ContentDialog）。
 * 用于展现模态强打断性交互面板（如保存确认、信息提示或简单文本输入框）。
 */
class ContentDialog : public UIElement {
public:
    ContentDialog();
    virtual ~ContentDialog() = default;

    virtual const char* GetClassName() const override { return "ContentDialog"; } // 获取类名

    void SetTitle(const std::string& title); // 设置弹出框大标题文本
    void SetMessage(const std::string& message); // 设置弹出正文详细文本内容
    void SetPrimaryButtonText(const std::string& text); // 设置主确认按钮的提示文本
    void SetSecondaryButtonText(const std::string& text); // 设置副辅助按钮的提示文本
    void SetCloseButtonText(const std::string& text); // 设置关闭/取消按钮的提示文本

    void SetInputEnabled(bool enabled, bool multiline = false); // 设定是否启用输入文本框以及是否开启多行模式
    void SetInputText(const std::string& text); // 手动设置输入文本框的默认字词
    std::string GetInputText() const; // 读取用户在文本输入框中输入的内容
    bool IsInputEnabled() const { return m_inputEnabled; } // 检查输入文本框是否被启用

    void Show(std::function<void(DialogResult)> callback = nullptr); // 展开模态对话框并注册点击返回回调
    void Hide(); // 关闭并收起对话框

    void InvalidateCard(); // 强制刷新并重新光栅化对话框卡片

    bool IsOpen() const { return m_isOpen; } // 检查对话框当前是否处于显示状态
    virtual bool IsModalOverlayOpen() const override { return m_isOpen; } // 标识当前属于模态显示层（会阻断背景窗口消息）

    virtual Size Measure(Size availableSize) override; // 计算对话框内部组件的理想物理大小
    virtual void Arrange(Rect finalRect) override; // 编排和定位确认按钮、文本框和消息文本位置
    virtual void Render(GraphicsContext& ctx) override; // 托管混合背景半透明遮罩与卡片的渲染
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制卡片本身的底色、阴影及描边线圈
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 绘制卡片上的子级控件（在 Overlay 弹出层中）
    virtual UIElement* HitTestOverlay(float x, float y) override; // 对弹出卡片区及背景进行命中碰撞测试
    virtual UIElement* OnHitTestOverlay(float x, float y) override; // 弹出穿透定位
    virtual bool NeedsOverlayHitTest() const override { return true; } // 声明必须响应悬浮层碰撞测试

    virtual bool OnAnimationTick() override; // 驱动对话框卡片从中心缩放弹出的平滑进入动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于展开/收拢过渡动画中
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 收集卡片动画造成的重绘脏矩形区域
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 递归收集包括子元素在内的动画脏矩形区域
    virtual void OnThemeChanged() override; // 响应主题颜色改变

    static void ShowMessageBox(UIElement* root, const std::string& title, const std::string& message,
                               std::function<void(DialogResult)> callback = nullptr); // 静态助手：快速弹出一个标准的信息提示框

    static void ShowInputBox(
        UIElement* root,
        const std::string& title,
        const std::string& message,
        const std::string& initialText,
        bool multiline,
        std::function<void(DialogResult, const std::string&)> callback); // 静态助手：快速弹出一个带输入文本框的提示输入框

private:
    void LayoutCardChildren(float scale); // 根据当前缩放比例，编排卡片内所有子控件大小位置

    std::string m_titleText = "Message"; // 标题文本缓存
    std::string m_messageText = "";      // 正文消息文本缓存
    std::string m_primaryText = "确定";  // 主按钮文字
    std::string m_secondaryText = "";    // 副按钮文字
    std::string m_closeText = "取消";     // 关闭取消按钮文字

    bool m_isOpen = false;               // 对话框开启展示状态
    bool m_inputEnabled = false;         // 输入框开启使能状态
    bool m_inputMultiline = false;       // 输入框是否为多行模式
    std::function<void(DialogResult)> m_callback = nullptr; // 动作确定后的回调函数

    std::shared_ptr<TextBlock> m_txtTitle;    // 标题文本组件
    std::shared_ptr<TextBlock> m_txtMessage;  // 消息文本组件
    std::shared_ptr<TextBox> m_inputBox;      // 输入文本框组件
    std::shared_ptr<Button> m_btnPrimary;     // 主操作按钮
    std::shared_ptr<Button> m_btnSecondary;   // 副操作按钮
    std::shared_ptr<Button> m_btnClose;       // 取消关闭按钮

    Rect m_dialogBounds;                 // 卡片排版边界矩形
    RenderLayer m_cardLayer;             // 对话框卡片离屏绘制图层
    bool m_cardCacheValid = false;       // 离屏缓存纹理是否依然有效无需重绘

    int m_animState = 0;                 // 动画状态（0=Closed, 1=Opening, 2=Opened, 3=Closing）
    std::chrono::steady_clock::time_point m_animStartTime; // 动画启动高精时间戳
    float m_animProgress = 0.0f;         // 动画当前运行进度值 (0.0f -> 1.0f)
};

} // namespace CUI
