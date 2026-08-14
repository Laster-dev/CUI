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

    virtual const char* GetClassName() const override { return "SplitButton"; }

    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;

protected:
    // 覆盖基类选项，使得单击左侧主区域时不会直接弹窗，而是触发常规的 Click 事件
    virtual bool OpensOnPrimaryPress() const override { return false; }

private:
    Rect PrimaryRect() const;
    bool m_pressInChevron = false;
};

} // namespace CUI
