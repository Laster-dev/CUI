#include "MenuBar.h"
#include "../window/Window.h"
#include <algorithm>

namespace CUI {

MenuBar::MenuBar() {
    SetProperty("height", Value(30.0f));
    SetProperty("background", Value("#252526"));
    SetProperty("color", Value("#CCCCCC"));
}

std::shared_ptr<ContextMenu> MenuBar::AddMenu(const std::string& title) {
    MenuBarItem item;
    item.title = title;
    item.dropDownMenu = std::make_shared<ContextMenu>();
    m_menus.push_back(item);
    return item.dropDownMenu;
}

Size MenuBar::Measure(Size availableSize) {
    float h = GetProperty("height").AsFloat(30.0f);
    m_desiredSize = Size(availableSize.width, h);
    return m_desiredSize;
}

void MenuBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    D2D1_COLOR_F defaultTextColor = D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f);
    D2D1_COLOR_F hoverBgColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f);
    D2D1_COLOR_F openBgColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.18f);

    float curX = m_bounds.x + 6.0f;

    for (size_t i = 0; i < m_menus.size(); ++i) {
        Size txtSize = ctx.MeasureText(m_menus[i].title, "Segoe UI", 12.0f);
        float itemW = txtSize.width + 16.0f;
        Rect itemRect(curX, m_bounds.y + 3.0f, itemW, m_bounds.height - 6.0f);
        m_menus[i].bounds = itemRect;

        bool isOpen = (static_cast<int>(i) == m_activeOpenIndex);
        bool isHover = (static_cast<int>(i) == m_hoveredIndex);

        if (isOpen) {
            ctx.FillRoundedRect(itemRect, 4.0f, openBgColor);
        } else if (isHover) {
            ctx.FillRoundedRect(itemRect, 4.0f, hoverBgColor);
        }

        ctx.DrawText(
            m_menus[i].title,
            itemRect,
            defaultTextColor,
            "Segoe UI",
            12.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER
        );

        curX += itemW + 4.0f;
    }
}

void MenuBar::OnMouseMove(Point pt) {
    int oldHover = m_hoveredIndex;
    m_hoveredIndex = -1;

    for (size_t i = 0; i < m_menus.size(); ++i) {
        if (m_menus[i].bounds.Contains(pt.x, pt.y)) {
            m_hoveredIndex = static_cast<int>(i);
            break;
        }
    }

    // If a menu is already open, hover over another item opens its dropdown instantly
    if (m_activeOpenIndex >= 0 && m_hoveredIndex >= 0 && m_hoveredIndex != m_activeOpenIndex) {
        // Force open new hovered menu dropdown
        m_activeOpenIndex = -1; // Reset to allow OnMouseDown to select new index
        OnMouseDown(pt);
    }
}

void MenuBar::OnMouseLeave() {
    m_hoveredIndex = -1;
}

void MenuBar::OnMouseDown(Point pt) {
    for (size_t i = 0; i < m_menus.size(); ++i) {
        if (m_menus[i].bounds.Contains(pt.x, pt.y)) {
            // If clicking same open menu, toggle close it
            if (m_activeOpenIndex == static_cast<int>(i)) {
                if (m_menus[i].dropDownMenu) {
                    m_menus[i].dropDownMenu->Hide();
                }
                m_activeOpenIndex = -1;
                return;
            }

            // Close previously open menu if any
            if (m_activeOpenIndex >= 0 && m_activeOpenIndex < static_cast<int>(m_menus.size())) {
                if (m_menus[m_activeOpenIndex].dropDownMenu) {
                    m_menus[m_activeOpenIndex].dropDownMenu->Hide();
                }
            }

            m_activeOpenIndex = static_cast<int>(i);
            auto menu = m_menus[i].dropDownMenu;
            if (menu) {
                // Find root Window size
                float winW = 1280.0f, winH = 800.0f;
                UIElement* root = GetParent();
                while (root && root->GetParent()) {
                    root = root->GetParent();
                }
                if (root) {
                    winW = root->GetBounds().width;
                    winH = root->GetBounds().height;
                }

                // Popup menu dropdown right below the MenuBar item
                menu->ShowAt(m_menus[i].bounds.x, m_bounds.y + m_bounds.height, winW, winH);
                
                // Find parent element chain to set active ContextMenu
                UIElement* curr = this;
                while (curr) {
                    curr->SetContextMenu(menu);
                    curr = curr->GetParent();
                }
            }
            return;
        }
    }
}

} // namespace CUI
