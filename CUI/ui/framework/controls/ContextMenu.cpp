#include "ContextMenu.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include <algorithm>
#include <cmath>

namespace CUI {

MenuItem::MenuItem() {
    SetText("");
    SetColorToken(ThemeTokenId::TextSecondary);
    SetColor(ThemeManager::Instance().GetColor("textSecondary"));
    SetFontSize(12.0f);
    SetFontFamily("微软雅黑");
    SetHeight(26.0f);
}

MenuItem::MenuItem(const std::string& text, std::function<void()> onClick) : MenuItem() {
    SetText(text);
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
        ctx.DrawLine(Point(m_bounds.x + 8.0f, lineY), Point(m_bounds.x + m_bounds.width - 8.0f, lineY), ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder), 1.0f);
        return;
    }

    bool enabled = IsEnabled();
    const bool lightTheme = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;
    if (m_isHovered && enabled) {
        D2D1_COLOR_F hover = ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentColor);
        hover.a = lightTheme ? 0.16f : 0.32f;
        ctx.FillRoundedRect(m_bounds, 3.0f, hover);
    }

    std::string text = GetText();
    std::string icon = GetIcon();
    std::string shortcut = GetShortcutText();
    std::string font = GetFontFamily();
    float fontSize = GetFontSize();

    D2D1_COLOR_F textColor = enabled
        ? ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary)
        : ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextMuted);

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
        D2D1_COLOR_F scColor = enabled ? ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextMuted) : textColor;
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

void MenuItem::OnMouseWheel(float delta) {
    // Forward wheel scrolling to the parent menu so nested hit-test returns
    // a MenuItem target but the menu still scrolls.
    if (m_parentMenu) {
        m_parentMenu->OnMouseWheel(delta);
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
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBackground(ThemeManager::Instance().GetColor("cardBackground"));
    SetBorderToken(ThemeTokenId::CardBorder);
    SetBorderBrush(ThemeManager::Instance().GetColor("cardBorder"));
    SetBorderThickness(1.0f);
    SetCornerRadius(4.0f);
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

namespace {
Rect ResolveMenuViewport(float windowW, float windowH) {
    if (windowW > 0.0f && windowH > 0.0f) {
        return Rect(0.0f, 0.0f, windowW, windowH);
    }
    return GetPopupViewportOrDefault();
}
} // namespace

void ContextMenu::RelayoutItems() {
    if (m_items.empty()) return;

    const float w = (m_itemWidth > 8.0f) ? (m_itemWidth - 8.0f) : 0.0f;
    float currentY = m_bounds.y + 4.0f - m_scrollOffset;

    for (auto& item : m_items) {
        float h = item->IsSeparator() ? 6.0f : 26.0f;
        item->Arrange(Rect(m_bounds.x + 4.0f, currentY, w, h));
        currentY += h;
    }
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
            std::string t = item->GetText();
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
    float totalH = 8.0f;

    for (auto& item : m_items) {
        if (item->IsSeparator()) {
            totalH += 6.0f;
        } else {
            totalH += 26.0f;
        }
    }

    const Rect viewport = ResolveMenuViewport(windowW, windowH);
    m_bounds = PlacePopupAtPoint(Point(x, y), itemW, totalH, viewport);

    m_itemWidth = itemW;
    m_contentHeight = totalH;
    m_scrollOffset = 0.0f;
    RelayoutItems();

    if (PopupHost* host = PopupHost::Current()) {
        host->Open(this);
    }
}

void ContextMenu::ShowSubMenuAt(Rect parentItemBounds, float windowW, float windowH) {
    m_windowWidth = windowW;
    m_windowHeight = windowH;
    m_isOpen = true;

    float itemW = 200.0f;
    float totalH = 8.0f;

    for (auto& item : m_items) {
        if (item->IsSeparator()) {
            totalH += 6.0f;
        } else {
            totalH += 26.0f;
        }
    }

    const Rect viewport = ResolveMenuViewport(windowW, windowH);
    Rect anchor(parentItemBounds.x + parentItemBounds.width - 2.0f, parentItemBounds.y - 4.0f, 0.0f, parentItemBounds.height + 8.0f);
    m_bounds = PlacePopupNearAnchor(anchor, itemW, totalH, viewport, 2.0f);

    m_itemWidth = itemW;
    m_contentHeight = totalH;
    m_scrollOffset = 0.0f;
    RelayoutItems();
}

void ContextMenu::Hide() {
    const bool wasOpen = m_isOpen;
    m_isOpen = false;
    if (m_activeSubMenu) {
        m_activeSubMenu->Hide();
        m_activeSubMenu = nullptr;
    }
    if (wasOpen) {
        if (PopupHost* host = PopupHost::Current()) {
            host->Close(this);
        }
    }
}

void ContextMenu::OnMouseWheel(float delta) {
    if (!m_isOpen || m_items.empty()) return;

    if (m_contentHeight <= m_bounds.height + 0.001f) return; // no overflow

    const float maxScroll = (std::max)(0.0f, m_contentHeight - m_bounds.height);
    if (maxScroll <= 0.001f) return;

    // delta > 0 means wheel up -> decrease scrollOffset (show earlier items).
    const float step = 26.0f;
    float next = m_scrollOffset - delta * step;
    next = (std::clamp)(next, 0.0f, maxScroll);

    if (std::abs(next - m_scrollOffset) <= 0.001f) return;
    m_scrollOffset = next;
    m_scrollbarAutoHide.NotifyActivity(this);
    RequestAnimationTicks();

    // Re-layout items so RenderPopup + HitTestOverlay stay consistent.
    RelayoutItems();

    // Scrolling usually invalidates submenu attachment; hide it for correctness.
    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        m_activeSubMenu->Hide();
        m_activeSubMenu = nullptr;
    }

    MarkRenderContentDirty();
}

bool ContextMenu::OnAnimationTick() {
    const float prev = m_scrollbarAutoHide.Opacity();
    const bool hideAnimating = m_scrollbarAutoHide.Tick(UIElement::GetAnimationDeltaSeconds());
    if (std::abs(prev - m_scrollbarAutoHide.Opacity()) > 0.001f) {
        MarkRenderContentDirty();
    }
    if (hideAnimating) {
        RequestAnimationTicks();
    }
    return hideAnimating;
}

bool ContextMenu::HasSelfAnimation() const {
    return m_scrollbarAutoHide.NeedsTicks();
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
    // ContextMenu is typically not in the visual tree; PopupHost paints via RenderPopup.
    // Keep this as a fallback when no host is active.
    if (PopupHost::Current() && m_isOpen) return;
    RenderPopup(ctx);
}

void ContextMenu::RenderPopup(GraphicsContext& ctx) {
    if (!m_isOpen || m_items.empty()) return;

    float radius = GetCornerRadius();

    // Draw ContextMenu Popup Box (Shadow & Background)
    D2D1_COLOR_F bg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, 1.0f);

    // Render Menu Items
    ctx.PushClip(m_bounds);
    for (auto& item : m_items) {
        item->Render(ctx);
    }

    // Scrollbar (visual only; wheel scrolling supported).
    {
        const float contentH = m_contentHeight;
        const float visibleH = m_bounds.height;
        const float maxScroll = (std::max)(0.0f, contentH - visibleH);
        if (maxScroll > 0.001f && visibleH > 0.001f && m_scrollbarAutoHide.IsDrawn()) {
            constexpr float kScrollW = 8.0f;
            const float trackX = m_bounds.x + m_bounds.width - kScrollW;
            Rect trackRect(trackX, m_bounds.y, kScrollW, visibleH);
            const float vis = m_scrollbarAutoHide.Opacity();

            D2D1_COLOR_F trackColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);
            trackColor.a = 0.35f * vis;
            ctx.DrawRoundedRect(trackRect, 4.0f, trackColor, 1.0f);

            const float thumbH = (std::max)(16.0f, (visibleH * visibleH) / contentH);
            const float travel = (std::max)(0.0f, visibleH - thumbH);
            const float thumbY = trackRect.y + (m_scrollOffset / maxScroll) * travel;

            Rect thumbRect(trackX + 1.5f, thumbY, kScrollW - 3.0f, thumbH);
            D2D1_COLOR_F thumbColor = ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentColor);
            thumbColor.a = 0.45f * vis;
            ctx.FillRoundedRect(thumbRect, 4.0f, thumbColor);
        }
    }

    ctx.PopClip();

    // Render Active Submenu if open
    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        m_activeSubMenu->RenderPopup(ctx);
    }
}

bool ContextMenu::HitDismissExempt(float x, float y) const {
    return GetTotalBounds().Contains(x, y);
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
