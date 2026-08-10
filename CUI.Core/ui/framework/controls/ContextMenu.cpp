#include "ContextMenu.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {
constexpr AnimationSpec kMenuHoverSpec{ 0.22f, 0.01f, 0.16f }; // cubic ease-out via maxDurationSeconds
} // namespace

MenuItem::MenuItem() {
    SetText("");
    SetColorToken(ThemeTokenId::TextSecondary);
    SetColor(ThemeManager::Instance().GetColor("textSecondary"));
    SetFontSize(16.0f);
    SetFontFamily("微软雅黑");
    SetFontWeight("Normal");
    SetHeight(ContextMenu::kItemHeight);
}

MenuItem::MenuItem(const std::string& text, std::function<void()> onClick) : MenuItem() {
    SetText(text);
    m_command = onClick;
}

Size MenuItem::Measure(Size availableSize) {
    (void)availableSize;
    if (m_isSeparator) {
        m_desiredSize = Size(180.0f, ContextMenu::kSeparatorHeight);
        return m_desiredSize;
    }
    GraphicsContext ctx;
    const float contentW = MeasurePreferredContentWidth(ctx);
    m_desiredSize = Size(contentW, ContextMenu::kItemHeight);
    return m_desiredSize;
}

float MenuItem::MeasurePreferredContentWidth(GraphicsContext& ctx) const {
    if (m_isSeparator) return 160.0f;

    const std::string font = GetFontFamily();
    const float fontSize = GetFontSize();
    const Size label = ctx.MeasureText(GetText(), font, fontSize, DWRITE_FONT_WEIGHT_NORMAL);

    // Left gutter always reserved for checkmark / icon.
    float left = 28.0f;
    if (!GetIcon().empty()) left += 20.0f;

    float right = 12.0f;
    if (HasSubMenu()) {
        right = 28.0f;
    } else if (!m_shortcutText.empty()) {
        const Size sc = ctx.MeasureText(m_shortcutText, font, 14.0f, DWRITE_FONT_WEIGHT_NORMAL);
        right = sc.width + 24.0f;
    }

    return left + label.width + right;
}

void MenuItem::OnRender(GraphicsContext& ctx) {
    if (m_isSeparator) {
        float lineY = m_bounds.y + m_bounds.height / 2.0f;
        ctx.DrawLine(Point(m_bounds.x + 8.0f, lineY), Point(m_bounds.x + m_bounds.width - 8.0f, lineY),
                     ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder), 1.0f);
        return;
    }

    bool enabled = IsEnabled();
    const bool lightTheme = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;
    const float hoverT = m_hoverAnim.Current();
    if (hoverT > 0.001f && enabled) {
        D2D1_COLOR_F hover = ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentColor);
        const float peak = lightTheme ? 0.16f : 0.32f;
        hover.a = peak * hoverT;
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

    float textLeft = m_bounds.x + 28.0f;
    if (m_isChecked) {
        Rect checkRect(m_bounds.x + 4.0f, m_bounds.y, 22.0f, m_bounds.height);
        ctx.DrawText("✓", checkRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_CENTER,
                     DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL);
    }
    if (!icon.empty()) {
        Rect iconRect(m_bounds.x + 26.0f, m_bounds.y, 20.0f, m_bounds.height);
        ctx.DrawText(icon, iconRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_CENTER,
                     DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL);
        textLeft += 20.0f;
    }

    float rightReserve = 12.0f;
    if (HasSubMenu()) {
        rightReserve = 24.0f;
    } else if (!shortcut.empty()) {
        const Size sc = ctx.MeasureText(shortcut, font, 14.0f, DWRITE_FONT_WEIGHT_NORMAL);
        rightReserve = sc.width + 20.0f;
    }

    Rect textRect(textLeft, m_bounds.y,
                  (std::max)(0.0f, m_bounds.x + m_bounds.width - textLeft - rightReserve),
                  m_bounds.height);
    ctx.DrawText(text, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING,
                 DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL);

    if (HasSubMenu()) {
        Rect arrowRect(m_bounds.x + m_bounds.width - 20.0f, m_bounds.y, 16.0f, m_bounds.height);
        ctx.DrawText(">", arrowRect, textColor, font, 14.0f, DWRITE_TEXT_ALIGNMENT_TRAILING,
                     DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL);
    } else if (!shortcut.empty()) {
        Rect shortcutRect(m_bounds.x + m_bounds.width - rightReserve, m_bounds.y,
                          rightReserve - 8.0f, m_bounds.height);
        D2D1_COLOR_F scColor = enabled ? ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextMuted) : textColor;
        ctx.DrawText(shortcut, shortcutRect, scColor, font, 14.0f, DWRITE_TEXT_ALIGNMENT_TRAILING,
                     DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL);
    }
}

void MenuItem::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    if (!IsEnabled()) return;
    // Down only opens submenu; command runs on mouse up.
    if (HasSubMenu() && m_parentMenu) {
        m_parentMenu->OpenSubMenuForItem(this);
    }
}

void MenuItem::OnMouseUp(Point pt) {
    // Avoid base OnClick: command lives in m_command / ExecuteCommand.
    if (IsPressed()) {
        m_isPressed = false;
        MarkRenderRectDirty(m_bounds);
    }
    if (!IsEnabled() || HasSubMenu() || m_isSeparator) return;
    if (m_bounds.Contains(pt.x, pt.y)) {
        ExecuteCommand();
    }
}

void MenuItem::OnMouseEnter() {
    Control::OnMouseEnter();
    m_hoverAnim.SetTarget(1.0f);
    RequestAnimationTicks();
    if (m_parentMenu) {
        m_parentMenu->MarkRenderContentDirty();
        m_parentMenu->RequestAnimationTicks();
    }
}

void MenuItem::OnMouseLeave() {
    Control::OnMouseLeave();
    m_hoverAnim.SetTarget(0.0f);
    RequestAnimationTicks();
    if (m_parentMenu) {
        m_parentMenu->MarkRenderContentDirty();
        m_parentMenu->RequestAnimationTicks();
    }
}

bool MenuItem::TickHoverAnimation(float dt) {
    return m_hoverAnim.Tick(dt, kMenuHoverSpec);
}

bool MenuItem::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    bool hover = TickHoverAnimation(UIElement::GetAnimationDeltaSeconds());
    if (hover) {
        MarkRenderRectDirty(m_bounds);
        if (m_parentMenu) m_parentMenu->MarkRenderContentDirty();
        RequestAnimationTicks();
    }
    return base || hover;
}

bool MenuItem::HasSelfAnimation() const {
    return Control::HasSelfAnimation() || m_hoverAnim.IsAnimating();
}

void MenuItem::OnMouseWheel(float delta) {
    if (m_parentMenu) {
        m_parentMenu->OnMouseWheel(delta);
    }
}

void MenuItem::ExecuteCommand() {
    // Dismiss the whole menu tree BEFORE running the command. Otherwise a
    // ContentDialog opened from a submenu item (e.g. 新建 → 子项) leaves the
    // root context menu painted on top of the modal.
    auto cmd = m_command;
    ContextMenu* menu = m_parentMenu;
    if (menu) {
        menu->DismissHierarchy();
    }
    if (cmd) {
        cmd();
    }
    OnClick().Invoke(this);
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
    subMenu->SetOwnerMenu(this);
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

float ContextMenu::ComputePreferredWidth() const {
    GraphicsContext ctx;
    float maxContent = kMinWidth - 8.0f;
    for (const auto& item : m_items) {
        if (!item || item->IsSeparator()) continue;
        maxContent = (std::max)(maxContent, item->MeasurePreferredContentWidth(ctx));
    }
    // Outer chrome: 4px inset each side.
    return (std::max)(kMinWidth, maxContent + 8.0f);
}

float ContextMenu::ComputeContentHeight() const {
    float totalH = kVerticalPad;
    for (const auto& item : m_items) {
        totalH += item->IsSeparator() ? kSeparatorHeight : kItemHeight;
    }
    return totalH;
}

void ContextMenu::RelayoutItems() {
    if (m_items.empty()) return;

    const float w = (m_itemWidth > 8.0f) ? (m_itemWidth - 8.0f) : 0.0f;
    float currentY = m_bounds.y + 4.0f - m_scrollOffset;

    for (auto& item : m_items) {
        float h = item->IsSeparator() ? kSeparatorHeight : kItemHeight;
        item->Arrange(Rect(m_bounds.x + 4.0f, currentY, w, h));
        currentY += h;
    }
}

void ContextMenu::BeginOpenAnimation() {
    if (!UIElement::AreAnimationsEnabled()) {
        m_openProgress = 1.0f;
        m_openAnimating = false;
        return;
    }
    m_openProgress = 0.0f;
    m_openAnimating = true;
    RequestAnimationTicks();
}

void ContextMenu::ShowAt(float x, float y, float windowW, float windowH) {
    m_popupPosition = Point(x, y);
    m_windowWidth = windowW;
    m_windowHeight = windowH;
    m_isOpen = true;

    const float itemW = ComputePreferredWidth();
    const float totalH = ComputeContentHeight();

    const Rect viewport = ResolveMenuViewport(windowW, windowH);
    m_bounds = PlacePopupAtPoint(Point(x, y), itemW, totalH, viewport);

    m_itemWidth = itemW;
    m_contentHeight = totalH;
    m_scrollOffset = 0.0f;
    RelayoutItems();
    BeginOpenAnimation();

    if (PopupHost* host = PopupHost::Current()) {
        host->Open(this);
    }
}

void ContextMenu::ShowSubMenuAt(Rect parentItemBounds, float windowW, float windowH) {
    m_windowWidth = windowW;
    m_windowHeight = windowH;
    m_isOpen = true;

    const float itemW = ComputePreferredWidth();
    const float totalH = ComputeContentHeight();

    const Rect viewport = ResolveMenuViewport(windowW, windowH);
    Rect anchor(parentItemBounds.x + parentItemBounds.width - 2.0f, parentItemBounds.y - 4.0f,
                0.0f, parentItemBounds.height + 8.0f);
    m_bounds = PlacePopupNearAnchor(anchor, itemW, totalH, viewport, 2.0f);

    m_itemWidth = itemW;
    m_contentHeight = totalH;
    m_scrollOffset = 0.0f;
    RelayoutItems();
    BeginOpenAnimation();
}

void ContextMenu::OpenSubMenuForItem(MenuItem* item) {
    if (!item || !item->HasSubMenu()) return;
    auto sub = item->GetSubMenu();
    if (m_activeSubMenu == sub && sub && sub->IsOpen()) return;
    if (m_activeSubMenu) {
        m_activeSubMenu->Hide();
        m_activeSubMenu = nullptr;
    }
    m_activeSubMenu = sub;
    m_activeSubMenu->ShowSubMenuAt(item->GetBounds(), m_windowWidth, m_windowHeight);
    MarkRenderContentDirty();
}

void ContextMenu::Hide() {
    const bool wasOpen = m_isOpen;
    m_isOpen = false;
    m_openAnimating = false;
    m_openProgress = 0.0f;
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

void ContextMenu::DismissHierarchy() {
    ContextMenu* root = this;
    while (root->m_ownerMenu) {
        root = root->m_ownerMenu;
    }
    root->Hide();
}

void ContextMenu::OnMouseWheel(float delta) {
    if (!m_isOpen || m_items.empty()) return;

    if (m_contentHeight <= m_bounds.height + 0.001f) return;

    const float maxScroll = (std::max)(0.0f, m_contentHeight - m_bounds.height);
    if (maxScroll <= 0.001f) return;

    const float step = kItemHeight;
    float next = m_scrollOffset - delta * step;
    next = (std::clamp)(next, 0.0f, maxScroll);

    if (std::abs(next - m_scrollOffset) <= 0.001f) return;
    m_scrollOffset = next;
    m_scrollbarAutoHide.NotifyActivity(this);
    RequestAnimationTicks();

    RelayoutItems();

    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        m_activeSubMenu->Hide();
        m_activeSubMenu = nullptr;
    }

    MarkRenderContentDirty();
}

bool ContextMenu::TickItemHoverAnimations() {
    const float dt = UIElement::GetAnimationDeltaSeconds();
    bool any = false;
    for (auto& item : m_items) {
        if (item && item->TickHoverAnimation(dt)) {
            any = true;
        }
    }
    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        if (m_activeSubMenu->TickItemHoverAnimations()) {
            any = true;
        }
    }
    if (any) {
        MarkRenderContentDirty();
    }
    return any;
}

bool ContextMenu::TickPopupAnimation() {
    bool any = TickItemHoverAnimations();
    if (m_openAnimating) {
        m_openProgress += UIElement::GetAnimationDeltaSeconds() / kOpenAnimSeconds;
        if (m_openProgress >= 1.0f) {
            m_openProgress = 1.0f;
            m_openAnimating = false;
        }
        MarkRenderContentDirty();
        any = true;
    }
    const float prev = m_scrollbarAutoHide.Opacity();
    const bool hideAnimating = m_scrollbarAutoHide.Tick(UIElement::GetAnimationDeltaSeconds());
    if (std::abs(prev - m_scrollbarAutoHide.Opacity()) > 0.001f) {
        MarkRenderContentDirty();
        any = true;
    }
    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        if (m_activeSubMenu->TickPopupAnimation()) any = true;
    }
    return any || hideAnimating;
}

bool ContextMenu::OnAnimationTick() {
    bool child = UIElement::OnAnimationTick();
    bool hover = TickItemHoverAnimations();
    bool open = false;
    if (m_openAnimating) {
        m_openProgress += UIElement::GetAnimationDeltaSeconds() / kOpenAnimSeconds;
        if (m_openProgress >= 1.0f) {
            m_openProgress = 1.0f;
            m_openAnimating = false;
        }
        MarkRenderContentDirty();
        open = true;
    }
    const float prev = m_scrollbarAutoHide.Opacity();
    const bool hideAnimating = m_scrollbarAutoHide.Tick(UIElement::GetAnimationDeltaSeconds());
    if (std::abs(prev - m_scrollbarAutoHide.Opacity()) > 0.001f) {
        MarkRenderContentDirty();
    }
    if (hover || hideAnimating || open) {
        RequestAnimationTicks();
    }
    return child || hover || hideAnimating || open;
}

bool ContextMenu::HasSelfAnimation() const {
    if (m_openAnimating) return true;
    if (m_scrollbarAutoHide.NeedsTicks()) return true;
    for (const auto& item : m_items) {
        if (item && item->HasSelfAnimation()) return true;
    }
    if (m_activeSubMenu && m_activeSubMenu->IsOpen() && m_activeSubMenu->HasSelfAnimation()) return true;
    return false;
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
    if (PopupHost::Current() && m_isOpen) return;
    RenderPopup(ctx);
}

void ContextMenu::RenderPopup(GraphicsContext& ctx) {
    if (!m_isOpen || m_items.empty()) return;

    float radius = GetCornerRadius();
    const float alpha = (std::clamp)(m_openProgress, 0.0f, 1.0f);
    const float t = 1.0f - (1.0f - alpha) * (1.0f - alpha);

    D2D1_COLOR_F bg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);

    const float slide = (1.0f - t) * 4.0f;
    Rect drawBounds(m_bounds.x, m_bounds.y + slide, m_bounds.width, m_bounds.height);

    ctx.PushOpacity(t);
    ctx.FillRoundedRect(drawBounds, radius, bg);
    ctx.DrawRoundedRect(drawBounds, radius, border, 1.0f);

    const float dy = drawBounds.y - m_bounds.y;
    if (std::abs(dy) > 0.01f) {
        for (auto& item : m_items) {
            Rect b = item->GetBounds();
            item->Arrange(Rect(b.x, b.y + dy, b.width, b.height));
        }
    }

    ctx.PushClip(drawBounds);
    for (auto& item : m_items) {
        item->Render(ctx);
    }

    {
        const float contentH = m_contentHeight;
        const float visibleH = drawBounds.height;
        const float maxScroll = (std::max)(0.0f, contentH - visibleH);
        if (maxScroll > 0.001f && visibleH > 0.001f && m_scrollbarAutoHide.IsDrawn()) {
            constexpr float kScrollW = 8.0f;
            const float trackX = drawBounds.x + drawBounds.width - kScrollW;
            Rect trackRect(trackX, drawBounds.y, kScrollW, visibleH);
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
    ctx.PopOpacity();

    if (std::abs(dy) > 0.01f) {
        RelayoutItems();
    }

    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        m_activeSubMenu->RenderPopup(ctx);
    }
}

bool ContextMenu::HitDismissExempt(float x, float y) const {
    return GetTotalBounds().Contains(x, y);
}

UIElement* ContextMenu::HitTestOverlay(float x, float y) {
    if (!m_isOpen || m_items.empty()) return nullptr;

    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        UIElement* subHit = m_activeSubMenu->HitTestOverlay(x, y);
        if (subHit) return subHit;
    }

    if (m_bounds.Contains(x, y)) {
        for (auto it = m_items.rbegin(); it != m_items.rend(); ++it) {
            if ((*it)->GetBounds().Contains(x, y)) {
                auto item = (*it);
                if (item->HasSubMenu()) {
                    OpenSubMenuForItem(item.get());
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
