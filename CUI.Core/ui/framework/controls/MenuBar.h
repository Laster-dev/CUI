#pragma once
#include "Control.h"
#include "ContextMenu.h"
#include "../animation/AnimationSystem.h"
#include <vector>
#include <string>
#include <memory>

namespace CUI {

struct MenuBarItem {
    std::string title;
    std::shared_ptr<ContextMenu> dropDownMenu;
    Rect bounds;
    AnimatedScalar hoverAnim{ 0.0f };
};

class MenuBar : public Control {
public:
    MenuBar();
    virtual ~MenuBar() = default;

    virtual const char* GetClassName() const override { return "MenuBar"; }

    std::shared_ptr<ContextMenu> AddMenu(const std::string& title);
    void ClearMenus() { m_menus.clear(); }
    void ClearActiveMenu() { CloseActiveMenu(); m_hoveredIndex = -1; }
    void ResetInteractionState();
    std::shared_ptr<ContextMenu> GetActiveDropDown() const {
        if (m_activeOpenIndex < 0 || m_activeOpenIndex >= static_cast<int>(m_menus.size())) {
            return nullptr;
        }
        auto menu = m_menus[static_cast<size_t>(m_activeOpenIndex)].dropDownMenu;
        if (menu && menu->IsOpen()) return menu;
        return nullptr;
    }

    float GetTotalWidth(GraphicsContext& ctx);

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    // Returns true when menu chrome needs a repaint.
    bool HandleMouseMove(Point pt);
    virtual void OnMouseLeave() override;
    virtual void OnBlur() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

private:
    void OpenMenu(int index);
    void CloseActiveMenu();
    void HideAllMenusExcept(int keepIndex);
    void InvalidateMenuChrome(int indexA, int indexB = -1);
    void SyncHoverAnimationTargets();

    std::vector<MenuBarItem> m_menus;
    int m_hoveredIndex = -1;
    int m_activeOpenIndex = -1;
    int m_pendingOpenIndex = -1;
};

} // namespace CUI
