#pragma once
#include "Control.h"
#include "ScrollbarAutoHide.h"
#include "../window/PopupHost.h"
#include "../animation/AnimationSystem.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace CUI {

class MenuItem : public Control {
public:
    MenuItem();
    MenuItem(const std::string& text, std::function<void()> onClick = nullptr);
    virtual ~MenuItem() = default;

    virtual const char* GetClassName() const override { return "MenuItem"; }
    virtual HCURSOR GetCursor() const override { return IsEnabled() ? LoadCursor(nullptr, IDC_HAND) : nullptr; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseEnter() override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    bool IsSeparator() const { return m_isSeparator; }
    void SetIsSeparator(bool isSep) { m_isSeparator = isSep; }

    bool IsChecked() const { return m_isChecked; }
    void SetChecked(bool checked) {
        if (m_isChecked == checked) return;
        m_isChecked = checked;
        MarkRenderContentDirty();
    }

    const std::string& GetShortcutText() const { return m_shortcutText; }
    void SetShortcutText(const std::string& shortcut) {
        m_shortcutText = shortcut;
        MarkRenderContentDirty();
    }

    std::shared_ptr<ContextMenu> GetSubMenu() const { return m_subMenu; }
    void SetSubMenu(std::shared_ptr<ContextMenu> subMenu) { m_subMenu = subMenu; }
    bool HasSubMenu() const { return m_subMenu != nullptr; }

    void SetParentContextMenu(ContextMenu* menu) { m_parentMenu = menu; }
    void ExecuteCommand();

    // Preferred content width (label + icon + shortcut/arrow), excluding outer menu chrome.
    float MeasurePreferredContentWidth(GraphicsContext& ctx) const;
    bool TickHoverAnimation(float dt);

private:
    bool m_isSeparator = false;
    bool m_isChecked = false;
    std::string m_shortcutText;
    std::function<void()> m_command;
    ContextMenu* m_parentMenu = nullptr;
    std::shared_ptr<ContextMenu> m_subMenu = nullptr;
    AnimatedScalar m_hoverAnim{ 0.0f };
};

class ContextMenu : public UIElement, public IPopup {
public:
    ContextMenu();
    virtual ~ContextMenu() = default;

    virtual const char* GetClassName() const override { return "ContextMenu"; }

    virtual bool ShouldClipToBounds() const override { return false; }
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* HitTestOverlay(float x, float y) override;

    std::shared_ptr<MenuItem> AddItem(const std::string& text, std::function<void()> onClick = nullptr);
    std::shared_ptr<MenuItem> AddItem(const std::string& text, const std::string& shortcut, std::function<void()> onClick = nullptr);
    std::shared_ptr<ContextMenu> AddSubMenu(const std::string& text);
    void AddSeparator();

    void ShowAt(float x, float y, float windowW = 0.0f, float windowH = 0.0f);
    void ShowSubMenuAt(Rect parentItemBounds, float windowW = 0.0f, float windowH = 0.0f);
    void OpenSubMenuForItem(MenuItem* item);
    void Hide();
    // Walk to the outermost open menu and hide it (and all submenus).
    void DismissHierarchy();
    bool IsOpen() const { return m_isOpen; }
    Rect GetTotalBounds() const;
    std::shared_ptr<ContextMenu> GetActiveSubMenu() const { return m_activeSubMenu; }
    ContextMenu* GetOwnerMenu() const { return m_ownerMenu; }
    void SetOwnerMenu(ContextMenu* owner) { m_ownerMenu = owner; }

    // IPopup
    virtual bool IsPopupOpen() const override { return m_isOpen; }
    virtual Rect GetPopupBounds() const override { return GetTotalBounds(); }
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { Hide(); }
    virtual bool TickPopupAnimation() override;

    virtual void OnBlur() override { Hide(); }
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    static constexpr float kItemHeight = 28.0f;
    static constexpr float kSeparatorHeight = 6.0f;
    static constexpr float kVerticalPad = 8.0f; // 4 top + 4 bottom
    static constexpr float kMinWidth = 180.0f;
    static constexpr float kOpenAnimSeconds = 0.06f;

private:
    std::vector<std::shared_ptr<MenuItem>> m_items;
    bool m_isOpen = false;
    Point m_popupPosition;
    float m_windowWidth = 0.0f;
    float m_windowHeight = 0.0f;
    int m_hoveredIndex = -1;
    std::shared_ptr<ContextMenu> m_activeSubMenu = nullptr;
    ContextMenu* m_ownerMenu = nullptr; // parent menu when this is a submenu

    // Scrolling support when content height exceeds visible height.
    float m_scrollOffset = 0.0f;
    float m_contentHeight = 0.0f;
    float m_itemWidth = 0.0f;
    ScrollbarAutoHide m_scrollbarAutoHide;

    // Open fade (~60ms).
    float m_openProgress = 0.0f;
    bool m_openAnimating = false;

    void RelayoutItems();
    float ComputePreferredWidth() const;
    float ComputeContentHeight() const;
    bool TickItemHoverAnimations();
    void BeginOpenAnimation();
};

} // namespace CUI
