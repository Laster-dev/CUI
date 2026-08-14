#pragma once
#include "Control.h"
#include "ScrollbarAutoHide.h"
#include "../window/PopupHost.h"
#include <vector>
#include <string>

namespace CUI {

/**
 * @brief 下拉单选组合框（ComboBox）。
 * 允许用户点击按钮展现一个带滚动条的浮动下拉列表，并从中选择一个值。实现了 IPopup 接口。
 */
class ComboBox : public Control, public IPopup {
public:
    ComboBox();
    virtual ~ComboBox() = default;

    virtual const char* GetClassName() const override { return "ComboBox"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; } // 获取悬浮交互鼠标样式

    virtual Size Measure(Size availableSize) override; // 计算自适应宽度并为右侧下拉箭头留出空间
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制正常状态下的边框、当前选中项文本及下拉箭头
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 渲染弹出的下拉列表区域
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，执行弹出/收拢动作并计算点击位置
    virtual bool OnKeyDown(int vkCode) override; // 响应键盘上下方向键切换选中项与 Esc 键关闭
    virtual bool AcceptsTabFocus() const override { return true; } // 支持键盘 Tab 导航聚焦
    virtual void OnBlur() override; // 失去焦点时自动折叠下拉列表
    virtual bool OnAnimationTick() override; // 驱动下拉框展开、收起以及箭头翻转动画
    virtual bool HasSelfAnimation() const override; // 检查是否依然处于展开/折叠过渡动画中
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 收集弹窗层本帧动画产生的脏矩形区域
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 递归收集包括子控件在内的动画脏矩形区域

    virtual bool ShouldClipToBounds() const override { return !m_isDropDownOpen; } // 展开下拉列表时不限制边界裁剪
    virtual UIElement* HitTest(float x, float y) override; // 命中测试定位
    virtual UIElement* HitTestOverlay(float x, float y) override; // 弹出层命中测试定位
    virtual void OnMouseWheel(float delta) override; // 响应滚轮，平移下拉菜单内容列表

    // IPopup 弹出层接口实现
    virtual bool IsPopupOpen() const override { return m_isDropDownOpen; } // 获取下拉层是否已展开
    virtual Rect GetPopupBounds() const override; // 获取弹出层在全局视口坐标系下的边界包络盒
    virtual bool HitDismissExempt(float x, float y) const override; // 判定点击该区域是否免于强制消退收起
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); } // 弹窗穿透命中定位
    virtual void RenderPopup(GraphicsContext& ctx) override; // 绘制具体的下拉菜单内部条目与背景
    virtual void OnLightDismiss() override { SetDropDownOpen(false); } // 轻点空白背景消退时触发收拢关闭

    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值

    void AddItem(const std::string& item); // 向列表末尾添加一个项文本
    void ClearItems(); // 清空列表中的所有项
    void SetItems(const std::string& itemsCsv); // 通过逗号分隔符 (CSV) 批量快速导入并设置列表项
    const std::vector<std::string>& GetItems() const { return m_items; } // 获取选项明细队列

    PropertyRef<int, PropertyId::SelectedIndex> SelectedIndex; // 选中行号的属性双向绑定代理

    int GetSelectedIndex() const { return m_selectedIndex; } // 获取当前选中项的索引位置（-1为未选）
    void SetSelectedIndex(int index); // 设置当前选中项的索引位置

    std::string GetSelectedItem() const; // 读取当前选中的条目文本内容

    void SetDropDownOpen(bool open); // 展开或折叠收拢下拉列表面板
    bool IsDropDownOpen() const { return m_isDropDownOpen; } // 查询当前下拉列表是否已展开

    Event<ComboBox*, int, const std::string&>& OnSelectionChanged() { return m_onSelectionChangedEvent; } // 选中更改事件派发中心

private:
    std::vector<std::string> m_items;                                   // 菜单选项内容队列
    int m_selectedIndex = -1;                                           // 被选中项行号索引
    bool m_isDropDownOpen = false;                                      // 当前列表是否已展开
    int m_hoveredIndex = -1;                                            // 鼠标在下拉弹出列表中悬停的行号
    float m_scrollOffset = 0.0f;                                        // 下拉菜单列表长内容时的内部垂直滚动平移量
    ScrollbarAutoHide m_scrollbarAutoHide;                              // 自动淡入淡出的滚动条托管器
    AnimatedScalar m_popupAnim{};                                       // 弹出底盘展开/折收透明度动画
    AnimatedScalar m_arrowAnim{};                                       // 右侧小指示箭头的翻转动画

    Event<ComboBox*, int, const std::string&> m_onSelectionChangedEvent; // 选中更改事件对象
};

} // namespace CUI
