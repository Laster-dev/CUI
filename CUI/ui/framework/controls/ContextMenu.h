#pragma once
#include "Control.h"
#include "../window/PopupHost.h"
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
    virtual void OnMouseEnter() override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseWheel(float delta) override;

    bool IsSeparator() const { return m_isSeparator; }
    void SetIsSeparator(bool isSep) { m_isSeparator = isSep; }

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

private:
    bool m_isSeparator = false;
    std::string m_shortcutText;
    std::function<void()> m_command;
    ContextMenu* m_parentMenu = nullptr;
    std::shared_ptr<ContextMenu> m_subMenu = nullptr;
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
    void Hide();
    bool IsOpen() const { return m_isOpen; }
    Rect GetTotalBounds() const;
    std::shared_ptr<ContextMenu> GetActiveSubMenu() const { return m_activeSubMenu; }

    // IPopup
    virtual bool IsPopupOpen() const override { return m_isOpen; }
    virtual Rect GetPopupBounds() const override { return GetTotalBounds(); }
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { Hide(); }

    virtual void OnBlur() override { Hide(); }
    virtual void OnMouseWheel(float delta) override;

private:
    std::vector<std::shared_ptr<MenuItem>> m_items;
    bool m_isOpen = false;
    Point m_popupPosition;
    float m_windowWidth = 0.0f;
    float m_windowHeight = 0.0f;
    int m_hoveredIndex = -1;
    std::shared_ptr<ContextMenu> m_activeSubMenu = nullptr;

    // Scrolling support when content height exceeds visible height.
    float m_scrollOffset = 0.0f;     // how much content is shifted up (in layout/DIP coords)
    float m_contentHeight = 0.0f;   // full, unclamped content height (computed in ShowAt)
    float m_itemWidth = 0.0f;        // current menu width used by Arrange

    void RelayoutItems();
};

} // namespace CUI
