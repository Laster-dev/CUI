#pragma once
#include "Button.h"
#include "../window/PopupHost.h"
#include <functional>
#include <string>
#include <vector>

namespace CUI {

struct ButtonFlyoutItem {
    std::string text;
    bool separator = false;
    bool enabled = true;
    std::function<void()> onClick;
};

class DropDownButton : public Button, public IPopup {
public:
    DropDownButton();
    explicit DropDownButton(const std::string& text);
    virtual ~DropDownButton();

    virtual const char* GetClassName() const override { return "DropDownButton"; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual void OnKeyDown(int vkCode) override;
    virtual void OnBlur() override;
    virtual void OnNavigatedFrom() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual bool ShouldClipToBounds() const override { return !m_isDropDownOpen; }
    virtual UIElement* HitTestOverlay(float x, float y) override;

    virtual bool IsPopupOpen() const override { return m_isDropDownOpen; }
    virtual Rect GetPopupBounds() const override;
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { SetDropDownOpen(false); }

    int AddItem(const std::string& text, std::function<void()> onClick = nullptr);
    void AddSeparator();
    void ClearItems();
    const std::vector<ButtonFlyoutItem>& GetItems() const { return m_items; }

    void SetDropDownOpen(bool open);
    bool IsDropDownOpen() const { return m_isDropDownOpen; }

    Event<DropDownButton*, int, const std::string&>& OnItemChosen() { return m_onItemChosenEvent; }

protected:
    static constexpr float kChevronSlot = 28.0f;
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
    bool m_isDropDownOpen = false;
    int m_hoverIndex = -1;
    AnimatedScalar m_popupAnim{};
    AnimatedScalar m_arrowAnim{};
    Event<DropDownButton*, int, const std::string&> m_onItemChosenEvent;
};

} // namespace CUI
