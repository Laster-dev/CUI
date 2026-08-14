#pragma once
#include "Button.h"
#include "../window/PopupHost.h"
#include <functional>
#include <string>
#include <vector>

namespace CUI {

/**
 * @brief 下拉菜单项配置结构体。
 */
struct ButtonFlyoutItem {
    std::string text;              ///< 菜单项展示文本
    bool separator = false;         ///< 是否为分隔线线段
    bool enabled = true;            ///< 该项是否可用
    std::function<void()> onClick;  ///< 单击该项的回调行为
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

    virtual const char* GetClassName() const override { return "DropDownButton"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual void OnBlur() override;
    virtual void OnNavigatedFrom() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual bool ShouldClipToBounds() const override { return !m_isDropDownOpen; }
    virtual UIElement* HitTestOverlay(float x, float y) override;

    // IPopup 弹出层接口规范实现
    virtual bool IsPopupOpen() const override { return m_isDropDownOpen; }
    virtual Rect GetPopupBounds() const override;
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { SetDropDownOpen(false); }

    /**
     * @brief 添加一个菜单按钮项。
     * @param text 菜单项文本。
     * @param onClick 单击回调函数。
     * @return 返回该项在列表中的索引位置。
     */
    int AddItem(const std::string& text, std::function<void()> onClick = nullptr);
    
    // 添加一条横向分割线段
    void AddSeparator();
    void ClearItems();
    const std::vector<ButtonFlyoutItem>& GetItems() const { return m_items; }

    PropertyRef<int, PropertyId::SelectedIndex> SelectedIndex;  ///< 菜单被选中的索引项双向绑定属性

    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int index);
    std::string GetSelectedItem() const;

    // 开启或折叠下拉弹出菜单层
    void SetDropDownOpen(bool open);
    bool IsDropDownOpen() const { return m_isDropDownOpen; }

    Event<DropDownButton*, int, const std::string&>& OnItemChosen() { return m_onItemChosenEvent; }

protected:
    static constexpr float kChevronSlot = 28.0f;
    static constexpr float kChevronGlyph = 12.0f;
    static constexpr float kItemH = 32.0f;
    static constexpr float kSepH = 8.0f;
    static constexpr float kMenuPad = 4.0f;

    Rect ChevronRect() const;
    Rect LabelRect() const;
    Rect MenuRect() const;
    float MenuContentHeight() const;
    int HitTestMenuItem(Point pt) const;
    bool HandleMenuMouseDown(Point pt);
    void EndPressWithoutClick();
    void MoveHighlight(int delta);
    void ActivateHighlighted();
    virtual bool OpensOnPrimaryPress() const { return true; }

    std::vector<ButtonFlyoutItem> m_items;
    int m_selectedIndex = -1;
    bool m_isDropDownOpen = false;
    int m_hoverIndex = -1;
    AnimatedScalar m_popupAnim{};
    AnimatedScalar m_arrowAnim{};
    Event<DropDownButton*, int, const std::string&> m_onItemChosenEvent;
};

} // namespace CUI
