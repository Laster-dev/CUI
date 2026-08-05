#include "MenuBar.h"
#include "../window/Window.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

MenuBar::MenuBar() {
    SetHeight(30.0f);
    SetBackgroundToken(ThemeTokenId::TitleBarBackground);
    SetHoverBackgroundToken(ThemeTokenId::TitleBarBackground);
    SetPressedBackgroundToken(ThemeTokenId::TitleBarBackground);
    SetColorToken(ThemeTokenId::TitleBarText);
}

std::shared_ptr<ContextMenu> MenuBar::AddMenu(const std::string& title) {
    MenuBarItem item;
    item.title = title;
    item.dropDownMenu = std::make_shared<ContextMenu>();
    m_menus.push_back(item);
    return item.dropDownMenu;
}

float MenuBar::GetTotalWidth(GraphicsContext& ctx) {
    float totalW = 12.0f;
    for (size_t i = 0; i < m_menus.size(); ++i) {
        Size txtSize = ctx.MeasureText(m_menus[i].title, "Segoe UI", 12.0f);
        totalW += txtSize.width + 20.0f;
    }
    return totalW;
}

Size MenuBar::Measure(Size availableSize) {
    float h = GetHeight(); if (h < 0) h = 30.0f;
    m_desiredSize = Size(availableSize.width, h);
    return m_desiredSize;
}

void MenuBar::InvalidateMenuChrome(int indexA, int indexB) {
    // MenuBar is composed inside TitleBar (not a layout child); always dirty the
    // full strip so previous open/hover pills cannot linger in the scene cache.
    if (!m_bounds.IsEmpty()) {
        MarkRenderRectDirty(m_bounds.Inflate(4.0f));
    }
    auto dirtyItem = [&](int index) {
        if (index < 0 || index >= static_cast<int>(m_menus.size())) {
            return;
        }
        if (!m_menus[index].bounds.IsEmpty()) {
            MarkRenderRectDirty(m_menus[index].bounds.Inflate(4.0f));
        }
    };
    dirtyItem(indexA);
    dirtyItem(indexB);
}

void MenuBar::HideAllMenusExcept(int keepIndex) {
    for (size_t i = 0; i < m_menus.size(); ++i) {
        if (static_cast<int>(i) == keepIndex) {
            continue;
        }
        if (m_menus[i].dropDownMenu && m_menus[i].dropDownMenu->IsOpen()) {
            m_menus[i].dropDownMenu->Hide();
        }
    }
}

void MenuBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const bool lightTheme = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;
    D2D1_COLOR_F defaultTextColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TitleBarText);
    D2D1_COLOR_F hoverBgColor = lightTheme
        ? D2D1::ColorF(ThemeManager::Instance().GetTokens().cardBorder.r, ThemeManager::Instance().GetTokens().cardBorder.g, ThemeManager::Instance().GetTokens().cardBorder.b, 0.12f)
        : D2D1::ColorF(ThemeManager::Instance().GetTokens().cardBorder.r, ThemeManager::Instance().GetTokens().cardBorder.g, ThemeManager::Instance().GetTokens().cardBorder.b, 0.18f);
    D2D1_COLOR_F openBgColor = lightTheme
        ? D2D1::ColorF(ThemeManager::Instance().GetTokens().accentColor.r, ThemeManager::Instance().GetTokens().accentColor.g, ThemeManager::Instance().GetTokens().accentColor.b, 0.10f)
        : D2D1::ColorF(ThemeManager::Instance().GetTokens().accentColor.r, ThemeManager::Instance().GetTokens().accentColor.g, ThemeManager::Instance().GetTokens().accentColor.b, 0.18f);

    float curX = m_bounds.x + 6.0f;

    for (size_t i = 0; i < m_menus.size(); ++i) {
        Size txtSize = ctx.MeasureText(m_menus[i].title, "Segoe UI", 12.0f);
        float itemW = txtSize.width + 16.0f;
        Rect itemRect(curX, m_bounds.y + 3.0f, itemW, m_bounds.height - 6.0f);
        m_menus[i].bounds = itemRect;

        // Open highlight follows active index only — never leave a previous pill lit
        // because another menu's IsOpen() lagged a frame.
        const bool isOpen = (static_cast<int>(i) == m_activeOpenIndex);
        const bool isHover = (static_cast<int>(i) == m_hoveredIndex) && !isOpen;

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

void MenuBar::OpenMenu(int index) {
    if (index < 0 || index >= static_cast<int>(m_menus.size())) return;

    const int previousOpen = m_activeOpenIndex;
    HideAllMenusExcept(index);

    m_activeOpenIndex = index;
    m_hoveredIndex = index;
    InvalidateMenuChrome(previousOpen, index);

    auto menu = m_menus[index].dropDownMenu;
    if (menu) {
        float winW = 1280.0f, winH = 800.0f;
        UIElement* root = GetParent();
        while (root && root->GetParent()) {
            root = root->GetParent();
        }
        if (root) {
            winW = root->GetBounds().width;
            winH = root->GetBounds().height;
        }

        menu->ShowAt(m_menus[index].bounds.x, m_bounds.y + m_bounds.height, winW, winH);

        UIElement* curr = this;
        while (curr) {
            curr->SetContextMenu(menu);
            curr = curr->GetParent();
        }
    }
}

void MenuBar::CloseActiveMenu() {
    const int previousOpen = m_activeOpenIndex;
    HideAllMenusExcept(-1);
    m_activeOpenIndex = -1;
    InvalidateMenuChrome(previousOpen, m_hoveredIndex);
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

    if (oldHover != m_hoveredIndex) {
        InvalidateMenuChrome(oldHover, m_hoveredIndex);
    }

    // If a menu is already open, hover over another item opens its dropdown instantly
    if (m_activeOpenIndex >= 0 && m_hoveredIndex >= 0 && m_hoveredIndex != m_activeOpenIndex) {
        OpenMenu(m_hoveredIndex);
    }
}

void MenuBar::OnMouseLeave() {
    const int oldHover = m_hoveredIndex;
    m_hoveredIndex = -1;
    if (oldHover >= 0) {
        InvalidateMenuChrome(oldHover, m_activeOpenIndex);
    }
}

void MenuBar::ResetInteractionState() {
    const int previousOpen = m_activeOpenIndex;
    const int previousHover = m_hoveredIndex;
    m_hoveredIndex = -1;
    CloseActiveMenu();
    m_isHovered = false;
    m_isPressed = false;
    m_isFocused = false;
    InvalidateMenuChrome(previousOpen, previousHover);
}

void MenuBar::OnBlur() {
    Control::OnBlur();
    ResetInteractionState();
}

void MenuBar::OnMouseDown(Point pt) {
    for (size_t i = 0; i < m_menus.size(); ++i) {
        if (m_menus[i].bounds.Contains(pt.x, pt.y)) {
            const bool isOpen = (m_activeOpenIndex == static_cast<int>(i));
            if (isOpen) {
                CloseActiveMenu();
                return;
            }
            OpenMenu(static_cast<int>(i));
            return;
        }
    }
}

} // namespace CUI
