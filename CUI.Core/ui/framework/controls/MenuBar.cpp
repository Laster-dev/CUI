#include "MenuBar.h"
#include "../window/Window.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

namespace {
constexpr AnimationSpec kMenuBarHoverSpec{ 0.22f, 0.01f, 0.16f };
} // namespace

MenuBar::MenuBar() {
    SetHeight(30.0f);
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetHoverBackgroundToken(ThemeTokenId::PaneBackground);
    SetPressedBackgroundToken(ThemeTokenId::PaneBackground);
    SetColorToken(ThemeTokenId::TextPrimary);
}

std::shared_ptr<ContextMenu> MenuBar::AddMenu(const std::string& title) {
    MenuBarItem item;
    item.title = title;
    item.dropDownMenu = std::make_shared<ContextMenu>();
    m_menus.push_back(std::move(item));
    return m_menus.back().dropDownMenu;
}

float MenuBar::GetTotalWidth(GraphicsContext& ctx) {
    float totalW = 12.0f;
    for (size_t i = 0; i < m_menus.size(); ++i) {
        Size txtSize = ctx.MeasureText(m_menus[i].title, "微软雅黑", 16.0f, DWRITE_FONT_WEIGHT_NORMAL);
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
    // MenuBar is composed inside TitleBar (not a layout child); stamp the dirty
    // rect onto the parent as well so CollectRenderDirtyRegion can see it.
    auto dirtyRect = [&](const Rect& r) {
        if (r.IsEmpty()) return;
        const Rect inflated = r.Inflate(4.0f);
        MarkRenderRectDirty(inflated);
        if (m_parent) {
            m_parent->MarkRenderRectDirty(inflated);
        }
    };
    if (!m_bounds.IsEmpty()) {
        dirtyRect(m_bounds);
    }
    auto dirtyItem = [&](int index) {
        if (index < 0 || index >= static_cast<int>(m_menus.size())) {
            return;
        }
        dirtyRect(m_menus[index].bounds);
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

void MenuBar::SyncHoverAnimationTargets() {
    for (size_t i = 0; i < m_menus.size(); ++i) {
        const bool isOpen = (static_cast<int>(i) == m_activeOpenIndex);
        const bool isHover = (static_cast<int>(i) == m_hoveredIndex) && !isOpen;
        // Open state stays lit via open paint path; hover anim only for hover pill.
        m_menus[i].hoverAnim.SetTarget(isHover ? 1.0f : 0.0f);
    }
}

void MenuBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const bool lightTheme = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;
    D2D1_COLOR_F defaultTextColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    const float hoverPeak = lightTheme ? 0.12f : 0.18f;
    D2D1_COLOR_F openBgColor = lightTheme
        ? D2D1::ColorF(ThemeManager::Instance().GetTokens().accentColor.r, ThemeManager::Instance().GetTokens().accentColor.g, ThemeManager::Instance().GetTokens().accentColor.b, 0.10f)
        : D2D1::ColorF(ThemeManager::Instance().GetTokens().accentColor.r, ThemeManager::Instance().GetTokens().accentColor.g, ThemeManager::Instance().GetTokens().accentColor.b, 0.18f);

    float curX = m_bounds.x + 6.0f;

    for (size_t i = 0; i < m_menus.size(); ++i) {
        Size txtSize = ctx.MeasureText(m_menus[i].title, "微软雅黑", 16.0f, DWRITE_FONT_WEIGHT_NORMAL);
        float itemW = txtSize.width + 16.0f;
        Rect itemRect(curX, m_bounds.y + 3.0f, itemW, m_bounds.height - 6.0f);
        m_menus[i].bounds = itemRect;

        const bool isOpen = (static_cast<int>(i) == m_activeOpenIndex);
        const float hoverT = m_menus[i].hoverAnim.Current();

        if (isOpen) {
            ctx.FillRoundedRect(itemRect, 4.0f, openBgColor);
        } else if (hoverT > 0.001f) {
            D2D1_COLOR_F hoverBgColor = D2D1::ColorF(
                ThemeManager::Instance().GetTokens().cardBorder.r,
                ThemeManager::Instance().GetTokens().cardBorder.g,
                ThemeManager::Instance().GetTokens().cardBorder.b,
                hoverPeak * hoverT);
            ctx.FillRoundedRect(itemRect, 4.0f, hoverBgColor);
        }

        ctx.DrawText(
            m_menus[i].title,
            itemRect,
            defaultTextColor,
            "微软雅黑",
            16.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_NORMAL
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
    SyncHoverAnimationTargets();
    InvalidateMenuChrome(previousOpen, index);
    RequestAnimationTicks();

    auto menu = m_menus[index].dropDownMenu;
    if (menu) {
        // When the dropdown closes (item click / Escape / light-dismiss), clear the
        // open highlight — MenuBar is not a layout child so tree walks often miss it.
        menu->SetClosedCallback([this, index]() {
            if (m_activeOpenIndex != index) return;
            const int previous = m_activeOpenIndex;
            m_activeOpenIndex = -1;
            m_hoveredIndex = -1;
            SyncHoverAnimationTargets();
            InvalidateMenuChrome(previous, -1);
            RequestAnimationTicks();
        });
        menu->ShowAt(m_menus[index].bounds.x, m_bounds.y + m_bounds.height);

        UIElement* curr = this;
        while (curr) {
            curr->SetContextMenu(menu);
            curr = curr->GetParent();
        }
    }
}

void MenuBar::CloseActiveMenu() {
    const int previousOpen = m_activeOpenIndex;
    // Clear open index before Hide so ClosedCallback does not double-invalidate.
    m_activeOpenIndex = -1;
    HideAllMenusExcept(-1);
    SyncHoverAnimationTargets();
    InvalidateMenuChrome(previousOpen, m_hoveredIndex);
    RequestAnimationTicks();
}

bool MenuBar::HandleMouseMove(Point pt) {
    int oldHover = m_hoveredIndex;
    m_hoveredIndex = -1;

    for (size_t i = 0; i < m_menus.size(); ++i) {
        if (m_menus[i].bounds.Contains(pt.x, pt.y)) {
            m_hoveredIndex = static_cast<int>(i);
            break;
        }
    }

    bool chromeDirty = (oldHover != m_hoveredIndex);
    if (chromeDirty) {
        SyncHoverAnimationTargets();
        InvalidateMenuChrome(oldHover, m_hoveredIndex);
        RequestAnimationTicks();
    }

    // If a menu is already open, hover over another item opens its dropdown instantly
    if (m_activeOpenIndex >= 0 && m_hoveredIndex >= 0 && m_hoveredIndex != m_activeOpenIndex) {
        OpenMenu(m_hoveredIndex);
        chromeDirty = true;
    }
    return chromeDirty;
}

void MenuBar::OnMouseMove(Point pt) {
    HandleMouseMove(pt);
}

void MenuBar::OnMouseLeave() {
    const int oldHover = m_hoveredIndex;
    m_hoveredIndex = -1;
    SyncHoverAnimationTargets();
    if (oldHover >= 0) {
        InvalidateMenuChrome(oldHover, m_activeOpenIndex);
        RequestAnimationTicks();
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
    SyncHoverAnimationTargets();
    InvalidateMenuChrome(previousOpen, previousHover);
    RequestAnimationTicks();
}

void MenuBar::OnBlur() {
    Control::OnBlur();
    ResetInteractionState();
}

void MenuBar::OnMouseDown(Point pt) {
    m_pendingOpenIndex = -1;
    for (size_t i = 0; i < m_menus.size(); ++i) {
        if (m_menus[i].bounds.Contains(pt.x, pt.y)) {
            // If a menu is already open, switch immediately (standard menu-bar UX).
            if (m_activeOpenIndex >= 0) {
                if (m_activeOpenIndex == static_cast<int>(i)) {
                    m_pendingOpenIndex = static_cast<int>(i); // close on up
                } else {
                    OpenMenu(static_cast<int>(i));
                }
                return;
            }
            m_pendingOpenIndex = static_cast<int>(i);
            return;
        }
    }
}

void MenuBar::OnMouseUp(Point pt) {
    Control::OnMouseUp(pt);
    if (m_pendingOpenIndex < 0) return;
    const int index = m_pendingOpenIndex;
    m_pendingOpenIndex = -1;
    if (index >= static_cast<int>(m_menus.size())) return;
    if (!m_menus[static_cast<size_t>(index)].bounds.Contains(pt.x, pt.y)) return;

    if (m_activeOpenIndex == index) {
        CloseActiveMenu();
    } else {
        OpenMenu(index);
    }
}

bool MenuBar::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    bool any = false;
    for (auto& item : m_menus) {
        if (item.hoverAnim.Tick(dt, kMenuBarHoverSpec)) {
            any = true;
        }
    }
    if (any) {
        if (!m_bounds.IsEmpty()) {
            MarkRenderRectDirty(m_bounds.Inflate(4.0f));
        }
        RequestAnimationTicks();
    }
    return base || any;
}

bool MenuBar::HasSelfAnimation() const {
    if (Control::HasSelfAnimation()) return true;
    for (const auto& item : m_menus) {
        if (item.hoverAnim.IsAnimating()) return true;
    }
    return false;
}

} // namespace CUI
