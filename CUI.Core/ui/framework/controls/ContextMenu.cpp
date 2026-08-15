#include "ContextMenu.h"
#include "../style/ThemeManager.h"
#include "../window/PopupPlacement.h"
#include "../window/MenuPopupWindow.h"
#include "../window/Dpi.h"
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
    SetFontWeight(CUI::FontWeight::Normal);
    SetHeight(ContextMenu::kItemHeight);
}

MenuItem::MenuItem(const std::string& text, std::function<void()> onClick) : MenuItem() {
    SetText(text);
    if (onClick) {
        SetCommand(std::make_shared<Command>(std::move(onClick)));
    }
}

MenuItem::~MenuItem() {
    if (m_ownsNativeIcon && m_nativeIcon) {
        DestroyIcon(m_nativeIcon);
        m_nativeIcon = nullptr;
    }
}

void MenuItem::SetNativeIcon(HICON icon, bool takeOwnership) {
    if (m_ownsNativeIcon && m_nativeIcon && m_nativeIcon != icon) {
        DestroyIcon(m_nativeIcon);
    }
    m_nativeIcon = icon;
    m_ownsNativeIcon = takeOwnership && icon != nullptr;
    MarkRenderContentDirty();
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
    } else if (m_nativeIcon) {
        const float iconSize = 16.0f;
        Rect iconRect(m_bounds.x + 6.0f, m_bounds.y + (m_bounds.height - iconSize) * 0.5f, iconSize, iconSize);
        ctx.DrawHIcon(m_nativeIcon, iconRect, enabled ? 1.0f : 0.45f);
    } else if (!icon.empty()) {
        const float iconSize = 16.0f;
        Rect iconRect(m_bounds.x + 6.0f, m_bounds.y + (m_bounds.height - iconSize) * 0.5f, iconSize, iconSize);
        ctx.DrawIcon(icon, iconRect, textColor, enabled ? 1.0f : 0.45f, font, fontSize);
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
        if (IsEnabled() && HasSubMenu()) {
            m_parentMenu->OpenSubMenuForItem(this);
        } else if (!IsSeparator()) {
            m_parentMenu->CloseActiveSubMenu();
        }
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

void MenuItem::SetHighlight(bool highlighted) {
    m_hoverAnim.SetTarget(highlighted ? 1.0f : 0.0f);
    RequestAnimationTicks();
    MarkRenderRectDirty(m_bounds);
}

void MenuItem::ExecuteCommand() {
    auto cmd = GetCommand();
    ContextMenu* menu = m_parentMenu;
    if (menu) {
        menu->DismissHierarchy();
    }
    if (cmd && cmd->CanExecute()) {
        cmd->Execute();
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
    SetCornerRadius(8.0f);
}

ContextMenu::~ContextMenu() = default;

std::shared_ptr<MenuItem> ContextMenu::AddItem(const std::string& text, std::function<void()> onClick) {
    auto cmd = onClick ? std::make_shared<Command>(std::move(onClick)) : nullptr;
    return AddItem(text, cmd);
}

std::shared_ptr<MenuItem> ContextMenu::AddItem(const std::string& text, const std::string& shortcut, std::function<void()> onClick) {
    auto cmd = onClick ? std::make_shared<Command>(std::move(onClick)) : nullptr;
    if (cmd && !shortcut.empty()) {
        cmd->SetGesture(shortcut);
    }
    return AddItem(text, shortcut, cmd);
}

std::shared_ptr<MenuItem> ContextMenu::AddItem(const std::string& text, std::shared_ptr<Command> command) {
    std::string shortcut;
    if (command && !command->GetGesture().IsEmpty()) {
        shortcut = command->GetGesture().ToDisplayString();
    }
    return AddItem(text, shortcut, std::move(command));
}

std::shared_ptr<MenuItem> ContextMenu::AddItem(const std::string& text, const std::string& shortcut, std::shared_ptr<Command> command) {
    auto item = std::make_shared<MenuItem>(text);
    item->SetParentContextMenu(this);
    if (command) {
        if (command->GetLabel().empty()) {
            command->SetLabel(text);
        }
        if (!shortcut.empty() && command->GetGesture().IsEmpty()) {
            command->SetGesture(shortcut);
        }
        item->SetCommand(std::move(command));
    }
    if (!shortcut.empty()) {
        item->SetShortcutText(shortcut);
    } else if (item->GetCommand() && !item->GetCommand()->GetGesture().IsEmpty()) {
        item->SetShortcutText(item->GetCommand()->GetGesture().ToDisplayString());
    }
    m_items.push_back(item);
    AddChild(item);
    return item;
}

std::shared_ptr<ContextMenu> ContextMenu::AddSubMenu(const std::string& text) {
    auto item = AddSubMenuItem(text);
    return item ? item->GetSubMenu() : nullptr;
}

std::shared_ptr<MenuItem> ContextMenu::AddSubMenuItem(const std::string& text) {
    auto item = std::make_shared<MenuItem>(text);
    auto subMenu = std::make_shared<ContextMenu>();
    subMenu->SetOwnerMenu(this);
    item->SetSubMenu(subMenu);
    item->SetParentContextMenu(this);
    m_items.push_back(item);
    AddChild(item);
    return item;
}

void ContextMenu::AddSeparator() {
    auto item = std::make_shared<MenuItem>();
    item->SetIsSeparator(true);
    m_items.push_back(item);
    AddChild(item);
}

void ContextMenu::ClearItems() {
    if (m_activeSubMenu) {
        m_activeSubMenu->Hide();
        m_activeSubMenu = nullptr;
    }
    for (auto& item : m_items) {
        if (item) RemoveChild(item);
    }
    m_items.clear();
    m_scrollOffset = 0.0f;
    m_contentHeight = 0.0f;
}

void ContextMenu::EnsurePopulated() {
    if (!m_lazyPopulate) return;
    ClearItems();
    m_lazyPopulate(*this);
}

namespace {
Rect ResolveMenuViewport(float windowW, float windowH) {
    if (windowW > 0.0f && windowH > 0.0f) {
        return Rect(0.0f, 0.0f, windowW, windowH);
    }
    return GetPopupViewportOrDefault();
}

Rect MonitorWorkAreaDip(::HWND hwnd) {
    if (!hwnd) return Rect(0.0f, 0.0f, 1920.0f, 1080.0f);
    HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(mon, &mi)) {
        return Rect(0.0f, 0.0f, 1920.0f, 1080.0f);
    }
    const float scale = GetDpiScaleForWindow(hwnd);
    const float s = (scale > 0.001f) ? scale : 1.0f;
    return Rect(
        static_cast<float>(mi.rcWork.left) / s,
        static_cast<float>(mi.rcWork.top) / s,
        static_cast<float>(mi.rcWork.right - mi.rcWork.left) / s,
        static_cast<float>(mi.rcWork.bottom - mi.rcWork.top) / s);
}

Point ClientDipToScreenDip(::HWND hwnd, float clientX, float clientY) {
    const float scale = GetDpiScaleForWindow(hwnd);
    const float s = (scale > 0.001f) ? scale : 1.0f;
    POINT pt{
        static_cast<LONG>(std::lround(clientX * s)),
        static_cast<LONG>(std::lround(clientY * s))
    };
    ClientToScreen(hwnd, &pt);
    return Point(static_cast<float>(pt.x) / s, static_cast<float>(pt.y) / s);
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
        m_openAnim.Reset(1.0f);
        return;
    }
    m_openAnim.Reset(0.0f);
    m_openAnim.SetTarget(1.0f);
    RequestAnimationTicks();
}

void ContextMenu::ShowAt(float x, float y, float windowW, float windowH) {
    m_popupPosition = Point(x, y);
    m_windowWidth = windowW;
    m_windowHeight = windowH;
    m_isOpen = true;
    m_hostedExternally = false;

    const float itemW = ComputePreferredWidth();
    const float totalH = ComputeContentHeight();

    ::HWND owner = nullptr;
    if (PopupHost* host = PopupHost::Current()) {
        owner = host->GetOwnerHwnd();
    }

    if (owner) {
        const Point screenPt = ClientDipToScreenDip(owner, x, y);
        const Rect work = MonitorWorkAreaDip(owner);
        const Rect placed = PlacePopupAtPoint(screenPt, itemW, totalH, work);

        m_bounds = Rect(0.0f, 0.0f, placed.width, placed.height);
        m_itemWidth = itemW;
        m_contentHeight = totalH;
        m_scrollOffset = 0.0f;
        RelayoutItems();
        BeginOpenAnimation();

        if (ShowOnExternalPopup(owner, Point(placed.x, placed.y), placed.width, placed.height)) {
            if (PopupHost* host = PopupHost::Current()) {
                host->Open(this);
            }
            return;
        }
    }

    // Fallback: in-window overlay (cannot extend outside the owner client).
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

    // Re-run lazy fill every open — Send To often needs a fresh WM_INITMENUPOPUP
    // at display time; the first build-time init is incomplete.
    EnsurePopulated();

    const float itemW = ComputePreferredWidth();
    const float totalH = ComputeContentHeight();
    if (m_items.empty() || totalH <= kVerticalPad + 0.5f) {
        m_isOpen = false;
        return;
    }

    ContextMenu* parent = m_ownerMenu;
    ::HWND owner = nullptr;
    if (PopupHost* host = PopupHost::Current()) {
        owner = host->GetOwnerHwnd();
    }

    // Dedicated popup ::HWND sized to this submenu only (avoids stretching the parent
    // surface into a tall empty background beside the root menu).
    if (owner && parent && parent->m_hostedExternally && parent->m_popupSurface) {
        Point screenPt = parent->m_popupSurface->ClientDipToScreenDip(
            parentItemBounds.x + parentItemBounds.width - 2.0f,
            parentItemBounds.y - 4.0f);
        const Rect work = MonitorWorkAreaDip(owner);
        const Rect placed = PlacePopupAtPoint(screenPt, itemW, totalH, work);

        m_bounds = Rect(0.0f, 0.0f, placed.width, placed.height);
        m_itemWidth = itemW;
        m_contentHeight = totalH;
        m_scrollOffset = 0.0f;
        RelayoutItems();
        BeginOpenAnimation();
        ShowOnExternalPopup(owner, Point(placed.x, placed.y), placed.width, placed.height);
        return;
    }

    const float sideX = parentItemBounds.x + parentItemBounds.width - 2.0f;
    const float topY = parentItemBounds.y - 4.0f;
    const Rect viewport = ResolveMenuViewport(windowW, windowH);
    m_bounds = PlacePopupAtPoint(Point(sideX, topY), itemW, totalH, viewport);
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
    if (m_hostedExternally && m_popupSurface) {
        m_popupSurface->Invalidate();
    }
}

void ContextMenu::CloseActiveSubMenu() {
    if (!m_activeSubMenu) return;
    m_activeSubMenu->Hide();
    m_activeSubMenu = nullptr;
    MarkRenderContentDirty();
    if (m_hostedExternally && m_popupSurface) {
        m_popupSurface->Invalidate();
    }
}

void ContextMenu::Hide() {
    const bool wasOpen = m_isOpen;
    m_isOpen = false;
    m_openAnim.Reset(0.0f);
    if (m_activeSubMenu) {
        m_activeSubMenu->Hide();
        m_activeSubMenu = nullptr;
    }
    if (m_popupSurface) {
        m_popupSurface->Hide();
    }
    m_hostedExternally = false;
    if (wasOpen && !m_ownerMenu) {
        if (PopupHost* host = PopupHost::Current()) {
            host->Close(this);
        }
        // Move callback out first so re-entrant Hide from the handler is a no-op.
        ClosedCallback closed = std::move(m_closedCallback);
        m_closedCallback = nullptr;
        if (closed) {
            closed();
        }
    }
}

ContextMenu* ContextMenu::GetRootMenu() {
    ContextMenu* root = this;
    while (root->m_ownerMenu) {
        root = root->m_ownerMenu;
    }
    return root;
}

bool ContextMenu::ShowOnExternalPopup(::HWND owner, Point screenDipTopLeft, float width, float height) {
    if (!owner) return false;
    if (!m_popupSurface) {
        m_popupSurface = std::make_unique<MenuPopupWindow>();
    }
    if (!m_popupSurface->Show(this, owner, screenDipTopLeft, Size(width, height))) {
        m_popupSurface.reset();
        m_hostedExternally = false;
        return false;
    }
    m_hostedExternally = true;
    return true;
}

void ContextMenu::OffsetPopupHierarchy(float dx, float dy) {
    m_bounds.x += dx;
    m_bounds.y += dy;
    RelayoutItems();
    if (m_activeSubMenu && m_activeSubMenu->IsOpen() && !m_activeSubMenu->IsExternallyHosted()) {
        m_activeSubMenu->OffsetPopupHierarchy(dx, dy);
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
    m_openAnim.SetTarget(m_isOpen ? 1.0f : 0.0f);
    if (m_openAnim.Tick(UIElement::GetAnimationDeltaSeconds(), PopupReveal::kSpec)) {
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
    if (any && m_hostedExternally && m_popupSurface) {
        m_popupSurface->Invalidate();
    }
    return any || hideAnimating;
}

bool ContextMenu::OnAnimationTick() {
    // PopupHost::TickAnimations already steps this menu; don't double-advance.
    if (PopupHost::Current() && m_isOpen) {
        return UIElement::OnAnimationTick();
    }
    return TickPopupAnimation() || UIElement::OnAnimationTick();
}

bool ContextMenu::HasSelfAnimation() const {
    if (m_openAnim.IsAnimating()) return true;
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
    if (m_activeSubMenu && m_activeSubMenu->IsOpen() && !m_activeSubMenu->IsExternallyHosted()) {
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
    const float progress = UIElement::AreAnimationsEnabled() ? m_openAnim.Current() : 1.0f;
    if (progress <= 0.001f) {
        return;
    }

    D2D1_COLOR_F bg = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBackground);
    D2D1_COLOR_F border = ThemeManager::Instance().GetFlatColor(ThemeTokenId::CardBorder);

    const Rect drawBounds = m_bounds;
    ctx.PushPopupReveal(drawBounds, progress, Point(drawBounds.x, drawBounds.y));
    ctx.FillRoundedRect(drawBounds, radius, bg);
    ctx.DrawRoundedRect(drawBounds, radius, border, 1.0f);

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
    ctx.PopPopupReveal();

    if (m_activeSubMenu && m_activeSubMenu->IsOpen() && !m_activeSubMenu->IsExternallyHosted()) {
        m_activeSubMenu->RenderPopup(ctx);
    }
}

bool ContextMenu::HitDismissExempt(float x, float y) const {
    // External popup ::HWND owns hit-testing; any click on the owner client dismisses.
    if (m_hostedExternally) return false;
    return GetTotalBounds().Contains(x, y);
}

UIElement* ContextMenu::HitTestOverlay(float x, float y) {
    if (!m_isOpen || m_items.empty()) return nullptr;

    // Externally hosted submenus live in their own ::HWND — do not hit-test them here.
    if (m_activeSubMenu && m_activeSubMenu->IsOpen() && !m_activeSubMenu->IsExternallyHosted()) {
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
                    CloseActiveSubMenu();
                }
                return item.get();
            }
        }
        return this;
    }

    return nullptr;
}

void ContextMenu::HighlightFirst() {
    int first = -1;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i] && !m_items[i]->IsSeparator() && m_items[i]->IsEnabled()) {
            first = i;
            break;
        }
    }
    m_hoveredIndex = first;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i]) {
            m_items[i]->SetHighlight(i == first);
        }
    }
}

bool ContextMenu::HandleKey(int vkCode) {
    if (!m_isOpen) {
        return false;
    }
    if (m_activeSubMenu && m_activeSubMenu->IsOpen()) {
        if (m_activeSubMenu->HandleKey(vkCode)) {
            return true;
        }
        if (vkCode == VK_LEFT || vkCode == VK_ESCAPE) {
            CloseActiveSubMenu();
            return true;
        }
    }

    auto isSelectable = [](const std::shared_ptr<MenuItem>& item) {
        return item && !item->IsSeparator() && item->IsEnabled();
    };

    auto moveHighlight = [&](int delta) {
        if (m_items.empty()) {
            return;
        }
        int start = m_hoveredIndex;
        int idx = start;
        for (int n = 0; n < static_cast<int>(m_items.size()); ++n) {
            idx += delta;
            if (idx < 0) {
                idx = static_cast<int>(m_items.size()) - 1;
            } else if (idx >= static_cast<int>(m_items.size())) {
                idx = 0;
            }
            if (isSelectable(m_items[idx])) {
                if (start >= 0 && start < static_cast<int>(m_items.size()) && m_items[start]) {
                    m_items[start]->SetHighlight(false);
                }
                m_hoveredIndex = idx;
                m_items[idx]->SetHighlight(true);
                CloseActiveSubMenu();
                return;
            }
        }
    };

    if (vkCode == VK_DOWN) {
        if (m_hoveredIndex < 0) {
            HighlightFirst();
        } else {
            moveHighlight(1);
        }
        return true;
    }
    if (vkCode == VK_UP) {
        if (m_hoveredIndex < 0) {
            HighlightFirst();
        } else {
            moveHighlight(-1);
        }
        return true;
    }
    if (vkCode == VK_HOME) {
        HighlightFirst();
        return true;
    }
    if (vkCode == VK_END) {
        m_hoveredIndex = -1;
        moveHighlight(-1);
        return true;
    }
    if (vkCode == VK_RIGHT) {
        if (m_hoveredIndex >= 0 && m_hoveredIndex < static_cast<int>(m_items.size())) {
            auto& item = m_items[m_hoveredIndex];
            if (item && item->HasSubMenu()) {
                OpenSubMenuForItem(item.get());
                if (m_activeSubMenu) {
                    m_activeSubMenu->HighlightFirst();
                }
                return true;
            }
        }
        return false;
    }
    if (vkCode == VK_LEFT) {
        if (m_ownerMenu) {
            Hide();
            return true;
        }
        return false;
    }
    if (vkCode == VK_RETURN || vkCode == VK_SPACE) {
        if (m_hoveredIndex >= 0 && m_hoveredIndex < static_cast<int>(m_items.size())) {
            auto& item = m_items[m_hoveredIndex];
            if (!isSelectable(item)) {
                return true;
            }
            if (item->HasSubMenu()) {
                OpenSubMenuForItem(item.get());
                if (m_activeSubMenu) {
                    m_activeSubMenu->HighlightFirst();
                }
            } else {
                item->ExecuteCommand();
            }
        }
        return true;
    }
    if (vkCode == VK_ESCAPE) {
        Hide();
        return true;
    }
    return false;
}

} // namespace CUI
