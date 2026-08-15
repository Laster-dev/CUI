#pragma once
#include "Control.h"

namespace CUI {

/**
 * @brief 标准按钮控件。
 * 触发用户点击行为，并内置流畅的水波纹（Ripple）点击反馈动画。
 */
class Button : public Control {
public:
    Button();
    explicit Button(const std::string& text);
    virtual ~Button() = default;

    virtual const char* GetClassName() const override { return "Button"; } // 获取类名
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获得交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 测算按钮大小宽高
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制按钮表面与文本
    virtual void OnMouseDown(Point pt) override; // 鼠标按下触发按下视觉态
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘空格与回车点击触发事件
    virtual bool AcceptsTabFocus() const override { return true; } // 支持键盘 Tab 导航聚焦
    virtual bool OnAnimationTick() override; // 驱动按下/聚焦视觉态动画帧更新
    virtual bool HasSelfAnimation() const override; // 检查是否存在未完成的视觉态动画

    void SetText(const std::string& text) { UIElement::SetText(text); } // 修改文字内容，触发重新测量重绘
    const std::string& GetText() const { return UIElement::GetText(); } // 获取按钮文字内容

protected:
    void DrawButtonFace(GraphicsContext& ctx, D2D1_COLOR_F bg, D2D1_COLOR_F border, float borderThickness); // 绘制按钮背景及线条描边
    void DrawButtonLabel(GraphicsContext& ctx, const Rect& textRect, DWRITE_TEXT_ALIGNMENT align); // 绘制按钮主体文本
};

} // namespace CUI
