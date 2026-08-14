#pragma once
#include "Button.h"
#include "../window/PopupHost.h"
#include <functional>
#include <string>
#include <vector>

namespace CUI {

/**
 * @brief 下拉菜单项配置结构体。
 * text: 菜单项展示文本。
 * separator: 是否为分隔线线段。
 * enabled: 该项是否可用。
 * onClick: 单击该项的回调行为。
 */
struct ButtonFlyoutItem {
    std::string text;               // 菜单项展示文本内容
    bool separator = false;         // 标记该项是否为一条横向分隔装饰线
    bool enabled = true;            // 控制此条目是否可被点选交互
    std::function<void()> onClick;  // 被点击选择时的动作回调函数
};

/**
 * @brief 下拉按钮。
 * 用户点击该按钮时，会弹出一个菜单供选择。继承自 Button 并实现 IPopup 接口以托管悬浮弹窗层。
 */
class DropDownButton : public Button, public IPopup {
public:
    DropDownButton();
    explicit DropDownButton(const std::string& text);
    virtual ~DropDownButton();

    virtual const char* GetClassName() const override { return "DropDownButton"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射读取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否包含对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值

    virtual Size Measure(Size availableSize) override; // 计算宽度并加入下拉箭头预留尺寸
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制静止态下拉按钮、分割线以及下拉方向箭头
    virtual void OnRenderOverlay(GraphicsContext& ctx) override; // 渲染弹出的浮空下拉菜单列表
    virtual void OnMouseDown(Point pt) override; // 鼠标按下，启动波纹或切换弹出菜单开启状态
    virtual void OnMouseMove(Point pt) override; // 鼠标移动，更新浮窗菜单项 Hover 焦点
    virtual void OnMouseUp(Point pt) override; // 鼠标抬起，触发对应菜单项点击
    virtual void OnMouseWheel(float delta) override; // 响应滚轮，可上下平移大菜单列表
    virtual bool OnKeyDown(int vkCode) override; // 响应方向键控制列表焦点与 Esc 键关闭
    virtual void OnBlur() override; // 失去焦点时，自动折叠关闭下拉弹出菜单
    virtual void OnNavigatedFrom() override; // 被切走页面时自动收起下拉菜单
    virtual bool OnAnimationTick() override; // 驱动菜单展开及箭头翻转动画步进
    virtual bool HasSelfAnimation() const override; // 检查是否存在未播放完毕的弹出与过渡动画
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override; // 收集下拉弹层动画发生的重绘脏区域
    virtual bool ShouldClipToBounds() const override { return !m_isDropDownOpen; } // 展开时不裁剪，允许弹出列表越界绘制
    virtual UIElement* HitTestOverlay(float x, float y) override; // 弹出菜单命中测试定位

    // IPopup 弹出层接口规范实现
    virtual bool IsPopupOpen() const override { return m_isDropDownOpen; } // 下拉弹出层目前是否正处于展开状态
    virtual Rect GetPopupBounds() const override; // 获取浮动下拉菜单在全局视口坐标下的边界包络盒
    virtual bool HitDismissExempt(float x, float y) const override; // 检查点击坐标是否落在下拉按钮身上以豁免点击消退
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); } // 弹窗层命中穿透测试定位
    virtual void RenderPopup(GraphicsContext& ctx) override; // 对弹出框进行实际渲染绘制
    virtual void OnLightDismiss() override { SetDropDownOpen(false); } // 轻点背景消退时触发关闭收起弹出层

    int AddItem(const std::string& text, std::function<void()> onClick = nullptr); // 尾部插入一条普通菜单文本选项并返回索引
    void AddSeparator(); // 尾部插入一条装饰横分割线
    void ClearItems(); // 清空旗下全部的菜单条目

    const std::vector<ButtonFlyoutItem>& GetItems() const { return m_items; } // 读取菜单项列表

    PropertyRef<int, PropertyId::SelectedIndex> SelectedIndex; // 绑定菜单被选中的索引项双向绑定属性代理

    int GetSelectedIndex() const { return m_selectedIndex; } // 获取当前选中项的索引位置
    void SetSelectedIndex(int index); // 设置选中项索引位置
    std::string GetSelectedItem() const; // 读取被选中项的字符串文本

    void SetDropDownOpen(bool open); // 控制下拉菜单列表的弹起与折叠收拢
    bool IsDropDownOpen() const { return m_isDropDownOpen; } // 查询当前是否处于展开状态

    Event<DropDownButton*, int, const std::string&>& OnItemChosen() { return m_onItemChosenEvent; } // 菜单项被点选确认的事件发布中心

protected:
    static constexpr float kChevronSlot = 28.0f; // 箭头在按钮内占用的水平宽度空间 (px)
    static constexpr float kChevronGlyph = 12.0f; // 下拉箭头几何图标的逻辑大小 (px)
    static constexpr float kItemH = 32.0f; // 菜单单行高度限制像素值 (px)
    static constexpr float kSepH = 8.0f; // 分割线高度占位像素值 (px)
    static constexpr float kMenuPad = 4.0f; // 菜单面板内边距留白 (px)

    Rect ChevronRect() const; // 获取箭头所在的局部绘制区域盒
    Rect LabelRect() const; // 获取左侧文字标签局部区域
    Rect MenuRect() const; // 获取弹出菜单视口区域
    float MenuContentHeight() const; // 计算整个菜单全部条目累加的排版高度
    int HitTestMenuItem(Point pt) const; // 测试并定位坐标落在哪个菜单条目的索引上
    bool HandleMenuMouseDown(Point pt); // 托管下拉菜单区域的点击按下逻辑
    void EndPressWithoutClick(); // 剥离交互而不触发点击重置状态
    void MoveHighlight(int delta); // 使用键盘流动高亮菜单项索引
    void ActivateHighlighted(); // 激活当前高亮选中的菜单选项
    virtual bool OpensOnPrimaryPress() const { return true; } // 规定是否在鼠标左键按下时立即开启下拉菜单

    std::vector<ButtonFlyoutItem> m_items;                              // 包含的所有选项明细队列
    int m_selectedIndex = -1;                                           // 选中项的行号索引（-1为未选）
    bool m_isDropDownOpen = false;                                      // 当前列表是否正处于弹开状态
    int m_hoverIndex = -1;                                              // 鼠标当前悬浮在下拉列表里的哪一行号
    AnimatedScalar m_popupAnim{};                                       // 菜单底盘展开收拢淡入渐变动效
    AnimatedScalar m_arrowAnim{};                                       // 箭头翻转旋转渐变动画
    Event<DropDownButton*, int, const std::string&> m_onItemChosenEvent; // 选项确立事件分发器
};

} // namespace CUI
