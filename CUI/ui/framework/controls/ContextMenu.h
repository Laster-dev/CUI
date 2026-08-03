#pragma once
#include "Control.h"
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

    bool IsSeparator() const { return m_isSeparator; }
    void SetIsSeparator(bool isSep) { m_isSeparator = isSep; }

    std::string GetShortcutText() const { return GetProperty("shortcutText").AsString(); }
    void SetShortcutText(const std::string& shortcut) { SetProperty("shortcutText", Value(shortcut)); }

    std::string GetIcon() const { return GetProperty("icon").AsString(); }
    void SetIcon(const std::string& icon) { SetProperty("icon", Value(icon)); }

    std::shared_ptr<ContextMenu> GetSubMenu() const { return m_subMenu; }
    void SetSubMenu(std::shared_ptr<ContextMenu> subMenu) { m_subMenu = subMenu; }
    bool HasSubMenu() const { return m_subMenu != nullptr; }

    void SetParentContextMenu(ContextMenu* menu) { m_parentMenu = menu; }
    void ExecuteCommand();

private:
    bool m_isSeparator = false;
    std::function<void()> m_command;
    ContextMenu* m_parentMenu = nullptr;
    std::shared_ptr<ContextMenu> m_subMenu = nullptr;
};

class ContextMenu : public UIElement {
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
    std::shared_ptr<ContextMenu> GetActiveSubMenu() const { return m_activeSubMenu; }

    virtual void OnBlur() override { Hide(); }

private:
    std::vector<std::shared_ptr<MenuItem>> m_items;
    bool m_isOpen = false;
    Point m_popupPosition;
    float m_windowWidth = 0.0f;
    float m_windowHeight = 0.0f;
    int m_hoveredIndex = -1;
    std::shared_ptr<ContextMenu> m_activeSubMenu = nullptr;
};

} // namespace CUI
