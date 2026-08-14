#pragma once
#include "DropDownButton.h"

namespace CUI {

/**
 * @brief 拆分按钮（SplitButton）。
 * 继承自 DropDownButton。按钮被纵向分隔线拆分为左侧的主触发区域（Primary Area）和右侧的下拉指示箭头区（Chevron Area）。
 * 左侧主区域触发传统的 Click 行为，右侧箭头区则触发弹出子菜单。
 */
class SplitButton : public DropDownButton {
public:
    SplitButton();
    explicit SplitButton(const std::string& text);
    virtual ~SplitButton() = default;

    virtual const char* GetClassName() const override { return "SplitButton"; } // 获取类名

    virtual void OnRender(GraphicsContext& ctx) override; // 绘制两态区分的分割线条及按钮底盘
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，根据落入区域（主区或箭头区）分别判定
    virtual void OnMouseUp(Point pt) override; // 鼠标松开，触发常规 Click 或弹出下拉列表
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘方向键及操作

protected:
    virtual bool OpensOnPrimaryPress() const override { return false; } // 规定单击左侧主区域时不会弹窗

private:
    Rect PrimaryRect() const; // 获取左侧主触发区域的局部包络盒

    bool m_pressInChevron = false; // 标记鼠标按下时是否落在了右侧的小箭头区域内
};

} // namespace CUI
