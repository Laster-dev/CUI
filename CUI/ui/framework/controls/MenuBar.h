#pragma once
#include "Control.h"
#include "ContextMenu.h"
#include <vector>
#include <string>
#include <memory>

namespace CUI {

struct MenuBarItem {
    std::string title;
    std::shared_ptr<ContextMenu> dropDownMenu;
    Rect bounds;
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

    float GetTotalWidth(GraphicsContext& ctx);

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnBlur() override;

private:
    void OpenMenu(int index);
    void CloseActiveMenu();
    void HideAllMenusExcept(int keepIndex);
    void InvalidateMenuChrome(int indexA, int indexB = -1);

    std::vector<MenuBarItem> m_menus;
    int m_hoveredIndex = -1;
    int m_activeOpenIndex = -1;
};

} // namespace CUI
