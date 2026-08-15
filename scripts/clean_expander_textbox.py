import os
import re

# 1. Expander.h clean rewrite
expander_h = '''#pragma once
#include "Control.h"

namespace CUI {

/**
 * @brief 折叠/展示的动画朝向方向。
 * Down: 向下展开折叠。
 * Up: 向上展开折叠。
 */
enum class ExpandDirection {
    Down = 0, // 向下折展方向
    Up = 1    // 向上折展方向
};

/**
 * @brief 折叠面板控件（Expander / CollapsePanel）。
 * 包含头部大文本、副标题文本（Subtitle）、以及右侧旋转的指示小箭头（Chevron）。
 * 点击头部会平滑地以高度渐变拉开或收拢下方的子内容（Content）。
 */
class Expander : public Control {
public:
    Expander();
    explicit Expander(const std::string& headerText);
    virtual ~Expander() = default;

    virtual const char* GetClassName() const override { return "Expander"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值
    virtual HCURSOR GetCursor() const override; // 获取悬浮交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 测算头部空间及下方折拢状态下的子内容排版占高
    virtual void Arrange(Rect finalRect) override; // 排布头部栏及下方面板裁剪内容
    virtual void Render(GraphicsContext& ctx) override; // 托管带剪切矩形（ClipRect）的折叠内容绘制以防溢出
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制头部底色、主副文字、分割线及小箭头图标
    virtual UIElement* HitTest(float x, float y) override; // 命中测试定位

    virtual void OnMouseMove(Point pt) override; // 鼠标滑动，高亮头部可交互背景
    virtual void OnMouseLeave() override; // 鼠标移出，还原头部高亮状态
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，捕获点击位置
    virtual void OnMouseUp(Point pt) override; // 鼠标松开，若在头部释放则触发展开/折拢切换
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘方向键及空格键切换折叠状态
    virtual bool OnAnimationTick() override; // 驱动下方面板折缩高度平滑变动动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于展开/折拢高度渐变动画中
    virtual void OnThemeChanged() override; // 响应主题更改，重新重绘

    struct ExpanderIsExpandedProperty {
        Expander* owner;
        ExpanderIsExpandedProperty& operator=(bool exp) { owner->SetIsExpanded(exp); return *this; }
        operator bool() const { return owner->GetIsExpanded(); }
        bool Get() const { return owner->GetIsExpanded(); }
    } IsExpanded{this};

    struct ExpanderContentProperty {
        Expander* owner;
        ExpanderContentProperty& operator=(std::shared_ptr<UIElement> c) { owner->SetContent(std::move(c)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner->GetContent(); }
        std::shared_ptr<UIElement> Get() const { return owner->GetContent(); }
        std::shared_ptr<UIElement> operator->() const { return owner->GetContent(); }
    } Content{this};

    struct ExpanderExpandDirectionProperty {
        Expander* owner;
        ExpanderExpandDirectionProperty& operator=(ExpandDirection d) { owner->SetExpandDirection(d); return *this; }
        operator ExpandDirection() const { return owner->GetExpandDirection(); }
        ExpandDirection Get() const { return owner->GetExpandDirection(); }
    } ExpandDirection{this};

    const std::string& GetHeader() const { return m_header; } // 获取头部主标题文本内容
    void SetHeader(const std::string& header); // 设置头部主标题文本内容

    const std::string& GetSubtitle() const { return m_subtitle; } // 获取头部副标题说明文本内容
    void SetSubtitle(const std::string& subtitle); // 设置头部副标题说明文本内容

    bool GetIsExpanded() const { return m_isExpanded; } // 检查当前是否处于展开状态
    void SetIsExpanded(bool expanded); // 设定是否展开面板并启动高度缩放过渡动画
    void SetExpanded(bool expanded) { SetIsExpanded(expanded); } // 兼容别名：设定是否展开

    ExpandDirection GetExpandDirection() const { return m_expandDirection; } // 获取折展的方向朝向
    void SetExpandDirection(ExpandDirection direction); // 设置折展的方向朝向

    std::shared_ptr<UIElement> GetContent() const { return m_content; } // 获取所承载展示的内容子元素
    void SetContent(std::shared_ptr<UIElement> content); // 放入要折叠展开的具体子控件节点

    Event<Expander*, bool>& OnExpandedChanged() { return m_onExpandedChangedEvent; } // 展开/折叠状态切换事件

private:
    std::string m_header;                      // 头部主标题文字
    std::string m_subtitle;                    // 头部说明副标题
    bool m_isExpanded = false;                 // 逻辑展开标志
    ExpandDirection m_expandDirection = ExpandDirection::Down; // 当前的展开生长朝向
    std::shared_ptr<UIElement> m_content;      // 折叠卡片内承载的具体内容面板

    AnimatedScalar m_expandAnim{};             // 展开进度动画过渡值 (0.0f 为折起, 1.0f 为展开)
    AnimatedScalar m_headerHoverAnim{};        // 头部光标移入/移出的平滑悬停微光动效
    float m_measuredContentHeight = 0.0f;      // 记录内容自排版测算出的最大物理高度

    Event<Expander*, bool> m_onExpandedChangedEvent; // 展开状态切换事件发布源
};

} // namespace CUI
'''

with open(r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Expander.h', 'w', encoding='utf-8') as f:
    f.write(expander_h)

# 2. Update TextBox.h
path_tb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBox.h'
with open(path_tb, 'r', encoding='utf-8') as f:
    tb = f.read()

# Replace methods to have distinct names
tb = tb.replace('bool IsPasswordMode() const { return m_isPasswordMode; }', 'bool GetIsPasswordMode() const { return m_isPasswordMode; }')
tb = tb.replace('bool IsPasswordRevealed() const { return m_isPasswordRevealed; }', 'bool GetIsPasswordRevealed() const { return m_isPasswordRevealed; }')
tb = tb.replace('bool IsReadOnly() const { return m_isReadOnly; }', 'bool GetIsReadOnly() const { return m_isReadOnly; }')
# Remove any duplicate struct declarations in TextBox.h
tb = re.sub(r'(\s*struct TextBox\w+Property\s*\{[\s\S]*?\}\s*\w+\{this\};)+', '', tb)

# Insert the properties cleanly before GetDisplayedText
props = '''    struct TextBoxIsReadOnlyProperty {
        TextBox* owner;
        TextBoxIsReadOnlyProperty& operator=(bool r) { owner->SetIsReadOnly(r); return *this; }
        operator bool() const { return owner->GetIsReadOnly(); }
        bool Get() const { return owner->GetIsReadOnly(); }
    } IsReadOnly{this};

    struct TextBoxIsPasswordModeProperty {
        TextBox* owner;
        TextBoxIsPasswordModeProperty& operator=(bool p) { owner->SetIsPasswordMode(p); return *this; }
        operator bool() const { return owner->GetIsPasswordMode(); }
        bool Get() const { return owner->GetIsPasswordMode(); }
    } IsPasswordMode{this};

    struct TextBoxShowRevealButtonProperty {
        TextBox* owner;
        TextBoxShowRevealButtonProperty& operator=(bool s) { owner->SetShowRevealButton(s); return *this; }
        operator bool() const { return owner->GetShowRevealButton(); }
        bool Get() const { return owner->GetShowRevealButton(); }
    } ShowRevealButton{this};

    struct TextBoxAcceptsReturnProperty {
        TextBox* owner;
        TextBoxAcceptsReturnProperty& operator=(bool a) { owner->SetAcceptsReturn(a); return *this; }
        operator bool() const { return owner->GetAcceptsReturn(); }
        bool Get() const { return owner->GetAcceptsReturn(); }
    } AcceptsReturn{this};

    struct TextBoxTextWrappingProperty {
        TextBox* owner;
        TextBoxTextWrappingProperty& operator=(bool w) { owner->SetTextWrapping(w); return *this; }
        operator bool() const { return owner->GetTextWrapping(); }
        bool Get() const { return owner->GetTextWrapping(); }
    } TextWrapping{this};

    struct TextBoxAllowDropProperty {
        TextBox* owner;
        TextBoxAllowDropProperty& operator=(bool d) { owner->SetAllowDrop(d); return *this; }
        operator bool() const { return owner->GetAllowDrop(); }
        bool Get() const { return owner->GetAllowDrop(); }
    } AllowDrop{this};

'''
tb = tb.replace('    virtual std::wstring GetDisplayedText() const;', props + '    virtual std::wstring GetDisplayedText() const;')

with open(path_tb, 'w', encoding='utf-8') as f:
    f.write(tb)

print("Rewrote Expander.h and TextBox.h cleanly.")
