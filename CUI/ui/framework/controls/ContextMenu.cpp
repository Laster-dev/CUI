#include "ContextMenu.h"
#include "../style/ThemeManager.h"

namespace CUI {

MenuItem::MenuItem() {
    SetProperty("text", Value(""));
    SetProperty("icon", Value(""));
    SetProperty("shortcutText", Value(""));
    SetProperty("theme.colorToken", Value("textSecondary"));
    SetProperty("color", Value(ThemeManager::Instance().GetColor("textSecondary")));
    SetProperty("fontSize", Value(12.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("height", Value(26.0f));
}

MenuItem::MenuItem(const std::string& text, std::function<void()> onClick) : MenuItem() {
    SetProperty("text", Value(text));
    m_command = onClick;
}

Size MenuItem::Measure(Size availableSize) {
    if (m_isSeparator) {
        m_desiredSize = Size(180.0f, 6.0f);
        return m_desiredSize;
    }
    m_desiredSize = Size(180.0f, 26.0f);
    return m_desiredSize;
}

void MenuItem::OnRender(GraphicsContext& ctx) {
    if (m_isSeparator) {
        float lineY = m_bounds.y + m_bounds.height / 2.0f;
        ctx.DrawLine(Point(m_bounds.x + 8.0f, lineY), Point(m_bounds.x + m_bounds.width - 8.0f, lineY), ThemeManager::Instance().GetColor("cardBorder"), 1.0f);
        return;
    }

    bool enabled = IsEnabled();
    const bool lightTheme = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;
    if (m_isHovered && enabled) {
        D2D1_COLOR_F hover = ThemeManager::Instance().GetColor("accentColor");
        hover.a = lightTheme ? 0.16f : 0.32f;
        ctx.FillRoundedRect(m_bounds, 3.0f, hover);
    }

    std::string text = GetProperty("text").AsString("");
    std::string icon = GetIcon();
    std::string shortcut = GetShortcutText();
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(12.0f);

    D2D1_COLOR_F textColor = enabled
        ? ThemeManager::Instance().GetColor("textPrimary")
        : ThemeManager::Instance().GetColor("textMuted");

    // Draw Icon if available
    float iconW = 24.0f;
    if (!icon.empty()) {
        Rect iconRect(m_bounds.x + 6.0f, m_bounds.y, iconW, m_bounds.height);
        ctx.DrawText(icon, iconRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // Draw Main Text
    Rect textRect(m_bounds.x + 10.0f + (icon.empty() ? 0.0f : iconW), m_bounds.y, m_bounds.width - 120.0f, m_bounds.height);
    ctx.DrawText(text, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Draw Shortcut Text or Submenu Right Arrow on Right End
    if (HasSubMenu()) {
        Rect arrowRect(m_bounds.x + m_bounds.width - 20.0f, m_bounds.y, 16.0f, m_bounds.height);
        ctx.DrawText(">", arrowRect, textColor, font, 11.0f, DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    } else if (!shortcut.empty()) {
        Rect shortcutRect(m_bounds.x + m_bounds.width - 125.0f, m_bounds.y, 115.0f, m_bounds.height);
        D2D1_COLOR_F scColor = enabled ? ThemeManager::Instance().GetColor("textMuted") : textColor;
        ctx.DrawText(shortcut, shortcutRect, scColor, font, 11.0f, DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

void MenuItem::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    if (IsEnabled() && !HasSubMenu()) {
        ExecuteCommand();
    }
}

void MenuItem::OnMouseEnter() {
    Control::OnMouseEnter();
    if (m_parentMenu) {
        m_parentMenu->MarkRenderContentDirty();
    }
}

void MenuItem::OnMouseLeave() {
    Control::OnMouseLeave();
    if (m_parentMenu) {
        m_parentMenu->MarkRenderContentDirty();
    }
}

void MenuItem::ExecuteCommand() {
    if (m_command) {
        m_command();
    }
    OnClick().Invoke(this);
    if (m_parentMenu) {
        m_parentMenu->Hide();
    }
}

// ---------------- ContextMenu ----------------

ContextMenu::ContextMenu() {
    SetProperty("theme.backgroundToken", Value("cardBackground"));
    SetProperty("background", Value(ThemeManager::Instance().GetColor("cardBackground")));
    SetProperty("theme.borderToken", Value("cardBorder"));
    SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor("cardBorder")));
    SetProperty("borderThickness", Value(1.0f));
    SetProperty("cornerRadius", Value(4.0f));
}

std::shared_ptr<MenuItem> ContextMenu::AddItem(const std::string& text, std::function<void()> onClick) {
    auto item = std::make_shared<MenuItem>(text, onClick);
    item->SetParentContextMenu(this);
    m_items.push_back(item);
    AddChild(item);
    return item;
}

std::shared_ptr<MenuItem> ContextMenu::AddItem(const std::string& text, const std::string& shortcut, std::function<void()> onClick) {
    auto item = std::make_shared<MenuItem>(text, onClick);
    item->SetShortcutText(shortcut);
    item->SetParentContextMenu(this);
    m_items.push_back(item);
    AddChild(item);
    return item;
}

std::shared_ptr<ContextMenu> ContextMenu::AddSubMenu(const std::string& text) {
    auto item = std::make_shared<MenuItem>(text);
    auto subMenu = std::make_shared<ContextMenu>();
    item->SetSubMenu(subMenu);
    item->SetParentContextMenu(this);
    m_items.push_back(item);
    AddChild(item);
    return subMenu;
}

void ContextMenu::AddSeparator() {
    auto item = std::make_shared<MenuItem>();
    item->SetIsSeparator(true);
    m_items.push_back(item);
    AddChild(item);
}

void ContextMenu::ShowAt(float x, float y, float windowW, float windowH) {
    m_popupPosition = Point(x, y);
    m_windowWidth = windowW;
    m_windowHeight = windowH;
    m_isOpen = true;

    // Calculate dynamic menu width based on item content and shortcuts
    float maxItemWidth = 220.0f;
    for (auto& item : m_items) {
        if (!item->IsSeparator()) {
            std::string t = item->GetProperty("text").AsString();
            std::string sc = item->GetShortcutText();
            float tLen = static_cast<float>(t.length()) * 7.5f;
            float scLen = static_cast<float>(sc.length()) * 7.5f;
            float reqW = tLen + scLen + 50.0f;
            if (reqW > maxItemWidth) {
                maxItemWidth = reqW;
            }
        }
    }
    float itemW = maxItemWidth;
    float totalH = 8.0f; // Padding top/bottom

    for (auto& item : m_items) {
        if (item->IsSeparator()) {
            totalH += 6.0f;
        } else {
            totalH += 26.0f;
        }
    }

    // Smart right-edge overflow check: flip to left of cursor
    float popupX = x;
    if (windowW > 0.0f && (x + itemW > windowW - 4.0f)) {
        popupX = x - itemW;
        if (popupX < 4.0f) popupX = (windowW > itemW + 8.0f) ? (windowW - itemW - 4.0f) : 4.0f;
    }

    // Smart bottom-edge overflow check: flip above cursor
    float popupY = y;
    if (windowH > 0.0f && (y + totalH > windowH - 4.0f)) {
        popupY = y - totalH;
        if (popupY < 4.0f) popupY = (windowH > totalH + 8.0f) ? (windowH - totalH - 4.0f) : 4.0f;
    }

    m_bounds = Rect(popupX, popupY, itemW, totalH);

    // Arrange items inside calculated bounds
    float currentY = popupY + 4.0f;
    for (auto& item : m_items) {
        float h = item->IsSeparator() ? 6.0f : 26.0f;
        item->Arrange(Rect(popupX + 4.0f, currentY, itemW - 8.0f, h));
        currentY += h;
    }
}

void ContextMenu::ShowSubMenuAt(Rect parentItemBounds, float windowW, float windowH) {
    m_windowWidth = windowW;
    m_windowHeight = windowH;
    m_isOpen = true;

    float itemW = 200.0f;
    float totalH = 8.0f; // Padding top/bottom

    for (auto& item : m_items) {
        if (item->IsSeparator()) {
            totalH += 6.0f;
        } else {
            totalH += 26.0f;
        }
    }

    // Target right side of parent item by default
    float popupX = parentItemBounds.x + parentItemBounds.width - 2.0f;
    // Right-edge overflow check: flip to LEFT of parent item!
    if (windowW > 0.0f && (popupX + itemW > windowW - 4.0f)) {
        popupX = parentItemBounds.x - itemW + 2.0f;
        if (popupX < 4.0f) popupX = 4.0f;
    }

    // Target top of parent item by default
    float popupY = parentItemBounds.y - 4.0f;
    // Bottom-edge overflow check: flip UPWARDS!
    if (windowH > 0.0f && (popupY + totalH > windowH - 4.0f)) {
        popupY = parentItemBounds.y + parentItemBounds.height - totalH + 4.0f;
        if (popupY < 4.0f) popupY = 4.0f;
    }

    m_bounds = Rect(popupX, popupY, itemW, totalH);

    float currentY = popupY + 4.0f;
    for (auto& item : m_items) {
        float h = item->IsSeparator() ? 6.0f : 26.0f;
        item->Arrange(Rect(popupX + 4.0f, currentY, itemW - 8.0f, h));
        currentY += h;
    }
}

void ContextMenu::Hide() {
    m_isOpen = false;
    if (m_activeSubMenu) {
        m_activeSubMenu->Hide();
        m_activeSubMenu = nullptr;
    }
}

Rect ContextMenu::GetTotalBounds() const {
    if (!m_isOpen) return Rect();
    Rect r = m_bounds;
    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        r = r.Union(m_activeSubMenu->GetTotalBounds());
    }
    return r;
}

void ContextMenu::OnRenderOverlay(GraphicsContext& ctx) {
    if (!m_isOpen || m_items.empty()) return;

    float radius = GetProperty("cornerRadius").AsFloat(4.0f);

    // Draw ContextMenu Popup Box (Shadow & Background)
    D2D1_COLOR_F bg = ResolveThemeColor("theme.backgroundToken", "cardBackground");
    D2D1_COLOR_F border = ResolveThemeColor("theme.borderToken", "cardBorder");

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, 1.0f);

    // Render Menu Items
    for (auto& item : m_items) {
        item->Render(ctx);
    }

    // Render Active Submenu if open
    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        m_activeSubMenu->OnRenderOverlay(ctx);
    }
}

UIElement* ContextMenu::HitTestOverlay(float x, float y) {
    if (!m_isOpen || m_items.empty()) return nullptr;

    // 1. Check open active submenu first!
    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        UIElement* subHit = m_activeSubMenu->HitTestOverlay(x, y);
        if (subHit) return subHit;
    }

    // 2. Check current context menu items
    if (m_bounds.Contains(x, y)) {
        for (auto it = m_items.rbegin(); it != m_items.rend(); ++it) {
            if ((*it)->GetBounds().Contains(x, y)) {
                auto item = (*it);
                if (item->HasSubMenu()) {
                    if (m_activeSubMenu != item->GetSubMenu()) {
                        if (m_activeSubMenu) m_activeSubMenu->Hide();
                        m_activeSubMenu = item->GetSubMenu();
                        m_activeSubMenu->ShowSubMenuAt(item->GetBounds(), m_windowWidth, m_windowHeight);
                    }
                } else if (!item->IsSeparator()) {
                    if (m_activeSubMenu) {
                        m_activeSubMenu->Hide();
                        m_activeSubMenu = nullptr;
                    }
                }
                return item.get();
            }
        }
        return this;
    }

    return nullptr;
}

} // namespace CUI
