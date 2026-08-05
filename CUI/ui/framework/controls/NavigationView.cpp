#include "NavigationView.h"
#include "../style/ThemeManager.h"
#include "../window/Dpi.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <windows.h>

namespace CUI {

namespace {
constexpr float kChromeButton = 40.0f;
constexpr float kTopNavHeight = 48.0f;

void StyleChromeButton(const std::shared_ptr<Button>& btn) {
    // Tokens only — Button paint resolves via ThemeManager / ResolveThemeColor.
    btn->SetBackgroundToken(ThemeTokenId::CardBackground);
    btn->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    btn->SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    btn->SetColorToken(ThemeTokenId::TextPrimary);
    btn->SetBorderToken(ThemeTokenId::CardBorder);
    btn->SetBorderThickness(0.0f);
    btn->SetWidth(kChromeButton);
    btn->SetHeight(kChromeButton);
    btn->SetCornerRadius(4.0f);
    btn->SetFontSize(16.0f);
    btn->SetPadding(Thickness(0, 0, 0, 0));
}
}

NavigationView::NavigationView() {
    SetBackgroundToken(ThemeTokenId::WindowBackground);
    SetPaneBackgroundToken(ThemeTokenId::PaneBackground);
    SetIndicatorColorToken(ThemeTokenId::AccentColor);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetSecondaryColorToken(ThemeTokenId::TextSecondary);

    BuildChrome();
    EnsureSettingsItem();
    m_paneWidthAnim.Reset(m_openPaneLength);
}

std::vector<PropertyMeta> NavigationView::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "paneTitle", "string", "外观", "Pane Title" });
    metas.push_back({ "header", "string", "外观", "Header" });
    metas.push_back({ "openPaneLength", "float", "外观", "Open Pane Length" });
    metas.push_back({ "compactPaneLength", "float", "外观", "Compact Pane Length" });
    return metas;
}

void NavigationView::BuildChrome() {
    m_btnBack = std::make_shared<Button>("←");
    StyleChromeButton(m_btnBack);
    m_btnBack->SetCornerRadius(20.0f);
    AddChild(m_btnBack);
    m_btnBack->OnClick().Connect([this](UIElement*) {
        if (IsOverlayMode() && m_isPaneOpen) {
            SetIsPaneOpen(false);
            return;
        }
        if (m_backEnabled) {
            m_backRequested.Invoke(this);
        }
    });

    m_btnToggle = std::make_shared<Button>("☰");
    StyleChromeButton(m_btnToggle);
    AddChild(m_btnToggle);
    m_btnToggle->OnClick().Connect([this](UIElement*) {
        TogglePane();
    });

    EnsureMenuScroll();
}

void NavigationView::EnsureMenuScroll() {
    if (m_menuScroll && m_menuHost) {
        return;
    }

    m_menuHost = std::make_shared<StackPanel>(Orientation::Vertical);
    m_menuHost->SetGap(2.0f);
    m_menuHost->SetOrientation(Orientation::Vertical);
    m_menuHost->SetAlign(Alignment::Stretch);

    m_menuScroll = std::make_shared<ScrollViewer>();
    m_menuScroll->SetBackground(D2D1::ColorF(0, 0, 0, 0));
    m_menuScroll->AddChild(m_menuHost);
    AddChild(m_menuScroll);
}

void NavigationView::EnsureAnimationsScheduled() {
    RequestAnimationTicks();
}

void NavigationView::EnsureContentZOrder() {
    // Content must stay at the back of the child list so pane chrome wins hit-testing.
    // (AddChild appends — without this, page content can steal clicks from nav items.)
    auto sendToBack = [this](const std::shared_ptr<UIElement>& node) {
        if (!node) {
            return;
        }
        auto it = std::find(m_children.begin(), m_children.end(), node);
        if (it == m_children.end()) {
            return;
        }
        m_children.erase(it);
        m_children.insert(m_children.begin(), node);
    };

    if (m_contentAnimating && m_contentNext) {
        sendToBack(m_contentNext);
    }
    if (m_content) {
        sendToBack(m_content);
    }

    // Overlay pane must paint and hit-test above content.
    if (!(IsOverlayMode() && m_isPaneOpen)) {
        return;
    }

    std::vector<std::shared_ptr<UIElement>> paneNodes;
    paneNodes.reserve(16);
    if (m_menuScroll) paneNodes.push_back(m_menuScroll);
    for (auto& item : m_footerItems) {
        paneNodes.push_back(std::static_pointer_cast<UIElement>(item));
        if (auto* nvi = dynamic_cast<NavigationViewItem*>(item.get())) {
            for (auto& child : nvi->MenuItems()) {
                paneNodes.push_back(std::static_pointer_cast<UIElement>(child));
            }
        }
    }
    if (m_settingsItem) paneNodes.push_back(m_settingsItem);
    if (m_btnBack) paneNodes.push_back(m_btnBack);
    if (m_btnToggle) paneNodes.push_back(m_btnToggle);
    if (m_autoSuggestBox) paneNodes.push_back(m_autoSuggestBox);
    if (m_paneFooter) paneNodes.push_back(m_paneFooter);

    std::unordered_set<UIElement*> seen;
    seen.reserve(paneNodes.size() * 2);
    for (auto& node : paneNodes) {
        if (!node) continue;
        if (node.get() == m_content.get() || node.get() == m_contentNext.get()) continue;
        if (!seen.insert(node.get()).second) continue;

        RemoveChild(node);
        AddChild(node);
    }
}

void NavigationView::EnsureSettingsItem() {
    if (m_settingsItem) {
        return;
    }
    m_settingsItem = std::make_shared<NavigationViewItem>("Settings", "⚙");
    m_settingsItem->SetTag("Settings");
    WireItem(m_settingsItem, false);
}

void NavigationView::WireItem(const std::shared_ptr<NavigationViewItemBase>& item, bool intoMenuScroll) {
    if (!item) {
        return;
    }
    item->SetOwner(this);
    EnsureMenuScroll();

    auto isChildOf = [](UIElement* parent, UIElement* child) -> bool {
        if (!parent || !child) return false;
        for (const auto& c : parent->GetChildren()) {
            if (c.get() == child) return true;
        }
        return false;
    };

    if (intoMenuScroll) {
        if (m_menuHost && !isChildOf(m_menuHost.get(), item.get())) {
            // Detach from NavigationView if previously attached.
            RemoveChild(item);
            m_menuHost->AddChild(item);
        }
    } else {
        if (!isChildOf(this, item.get())) {
            if (m_menuHost) {
                m_menuHost->RemoveChild(item);
            }
            AddChild(item);
        }
    }

    if (auto* nvi = dynamic_cast<NavigationViewItem*>(item.get())) {
        nvi->OnInvoked().Clear();
        nvi->OnInvoked().Connect([this](NavigationViewItem* invoked) {
            NotifyItemInvoked(invoked);
        });
        nvi->OnExpandChanged().Clear();
        nvi->OnExpandChanged().Connect([this](NavigationViewItem*) {
            SyncMenuHostChildren();
            RelayoutChildren();
            MarkRenderContentDirty();
        });
        for (auto& child : nvi->MenuItems()) {
            WireItem(child, intoMenuScroll);
        }
    }
}

void NavigationView::AddMenuItem(const std::shared_ptr<NavigationViewItemBase>& item) {
    if (!item) {
        return;
    }
    WireItem(item, true);
    m_menuItems.push_back(item);
    SyncMenuHostChildren();
    RelayoutChildren();
    MarkRenderContentDirty();
}

void NavigationView::ClearMenuItems() {
    EnsureMenuScroll();
    for (auto& item : m_menuItems) {
        auto removeDeep = [&](auto&& self, const std::shared_ptr<NavigationViewItemBase>& node) -> void {
            if (!node) return;
            if (m_menuHost) {
                m_menuHost->RemoveChild(node);
            }
            RemoveChild(node);
            if (auto* nvi = dynamic_cast<NavigationViewItem*>(node.get())) {
                for (auto& child : nvi->MenuItems()) {
                    self(self, child);
                }
            }
        };
        removeDeep(removeDeep, item);
    }
    m_menuItems.clear();
    SyncMenuHostChildren();
    RelayoutChildren();
}

void NavigationView::AddFooterMenuItem(const std::shared_ptr<NavigationViewItemBase>& item) {
    if (!item) {
        return;
    }
    WireItem(item, false);
    m_footerItems.push_back(item);
    RelayoutChildren();
    MarkRenderContentDirty();
}

void NavigationView::ClearFooterMenuItems() {
    for (auto& item : m_footerItems) {
        RemoveChild(item);
        auto removeDeep = [&](auto&& self, const std::shared_ptr<NavigationViewItemBase>& node) -> void {
            if (auto* nvi = dynamic_cast<NavigationViewItem*>(node.get())) {
                for (auto& child : nvi->MenuItems()) {
                    RemoveChild(child);
                    self(self, child);
                }
            }
        };
        removeDeep(removeDeep, item);
    }
    m_footerItems.clear();
    RelayoutChildren();
}

void NavigationView::SyncMenuHostChildren() {
    EnsureMenuScroll();
    if (!m_menuHost) {
        return;
    }

    // Update expand/collapse visibility only — do NOT rebuild the child list every
    // RelayoutChildren call (that was thrashing ScrollViewer and causing scroll jank).
    std::vector<NavigationViewItemBase*> visible;
    for (auto& item : m_menuItems) {
        CollectVisibleFrom(item, visible);
    }

    std::unordered_set<UIElement*> visibleSet;
    visibleSet.reserve(visible.size() * 2);
    for (auto* base : visible) {
        visibleSet.insert(base);
    }

    auto walkAll = [&](auto&& self, const std::shared_ptr<NavigationViewItemBase>& node) -> void {
        if (!node) return;
        const bool show = visibleSet.count(node.get()) > 0;
        node->SetVisibility(show ? Visibility::Visible : Visibility::Collapsed);
        if (auto* nvi = dynamic_cast<NavigationViewItem*>(node.get())) {
            for (auto& child : nvi->MenuItems()) {
                self(self, child);
            }
        }
    };
    for (auto& item : m_menuItems) {
        walkAll(walkAll, item);
    }

    // Rebuild only when host is empty or missing nodes (first populate / after Clear).
    std::vector<std::shared_ptr<NavigationViewItemBase>> flat;
    auto flatten = [&](auto&& self, const std::shared_ptr<NavigationViewItemBase>& node) -> void {
        if (!node) return;
        flat.push_back(node);
        if (auto* nvi = dynamic_cast<NavigationViewItem*>(node.get())) {
            for (auto& child : nvi->MenuItems()) {
                self(self, child);
            }
        }
    };
    for (auto& item : m_menuItems) {
        flatten(flatten, item);
    }

    bool needsRebuild = m_menuHost->GetChildren().size() != flat.size();
    if (!needsRebuild) {
        const auto& kids = m_menuHost->GetChildren();
        for (size_t i = 0; i < flat.size(); ++i) {
            if (kids[i].get() != flat[i].get()) {
                needsRebuild = true;
                break;
            }
        }
    }

    if (needsRebuild) {
        m_menuHost->ClearChildren();
        for (auto& node : flat) {
            RemoveChild(node);
            m_menuHost->AddChild(node);
        }
    }
}

void NavigationView::SetIsSettingsVisible(bool visible) {
    m_settingsVisible = visible;
    if (m_settingsItem) {
        m_settingsItem->SetVisibility(visible ? Visibility::Visible : Visibility::Collapsed);
    }
    RelayoutChildren();
}

void NavigationView::SetPaneDisplayMode(NavigationViewPaneDisplayMode mode) {
    if (m_paneDisplayMode == mode) {
        return;
    }
    m_paneDisplayMode = mode;
    UpdateAdaptiveLayout(m_bounds.width);
    RelayoutChildren();
    MarkRenderContentDirty();
}

void NavigationView::SetIsPaneOpen(bool open) {
    if (m_isPaneOpen == open) {
        return;
    }
    m_isPaneOpen = open;
    m_paneWidthAnim.SetTarget(TargetPaneWidth());
    EnsureAnimationsScheduled();
    UpdateChildCompactFlags();
    RelayoutChildren();
    EnsureContentZOrder();
    MarkRenderContentDirty();
    if (open) {
        m_paneOpened.Invoke(this);
    } else {
        m_paneClosed.Invoke(this);
    }
}

void NavigationView::TogglePane() {
    SetIsPaneOpen(!m_isPaneOpen);
}

void NavigationView::SetOpenPaneLength(float length) {
    m_openPaneLength = (std::max)(0.0f, length);
    m_paneWidthAnim.SetTarget(TargetPaneWidth());
    RelayoutChildren();
}

void NavigationView::SetCompactPaneLength(float length) {
    m_compactPaneLength = (std::max)(0.0f, length);
    m_paneWidthAnim.SetTarget(TargetPaneWidth());
    RelayoutChildren();
}

void NavigationView::SetContent(const std::shared_ptr<UIElement>& content) {
    if (!content) {
        return;
    }

    // If an animation is in-flight, cancel it to keep state consistent.
    if (m_contentAnimating) {
        if (m_contentNext) {
            RemoveChild(m_contentNext);
        }
        m_contentNext.reset();
        m_contentAnimating = false;
        m_contentFadeAnim.Reset(1.0f);
        // m_content still references the previous page (already removed from tree).
    }

    if (m_content == content) {
        return;
    }

    // First content.
    if (!m_content) {
        m_content = content;
        bool alreadyChild = false;
        for (const auto& child : GetChildren()) {
            if (child.get() == content.get()) {
                alreadyChild = true;
                break;
            }
        }
        if (!alreadyChild) {
            AddChild(m_content);
        }
        RelayoutChildren();
        EnsureContentZOrder();
        MarkRenderRectDirty(GetContentAreaRect());
        MarkRenderContentDirty();
        return;
    }

    // Fade new page in; remove old from tree immediately to avoid ghost frames.
    if (m_content) {
        RemoveChild(m_content);
    }
    m_contentNext = content;
    m_contentAnimating = true;
    m_contentFadeAnim.Reset(0.0f);
    m_contentFadeAnim.SetTarget(1.0f);
    m_contentNext->SetOpacity(0.0f);

    AddChild(m_contentNext);
    RelayoutChildren();
    EnsureContentZOrder();
    EnsureAnimationsScheduled();
    MarkRenderRectDirty(GetContentAreaRect());
    MarkRenderContentDirty();
}

void NavigationView::SetHeader(const std::string& header) {
    m_header = header;
    RelayoutChildren();
    MarkRenderContentDirty();
}

void NavigationView::SetAlwaysShowHeader(bool always) {
    m_alwaysShowHeader = always;
    RelayoutChildren();
    MarkRenderContentDirty();
}

void NavigationView::SetPaneTitle(const std::string& title) {
    m_paneTitle = title;
    MarkRenderContentDirty();
}

void NavigationView::SetPaneFooter(const std::shared_ptr<UIElement>& footer) {
    if (m_paneFooter == footer) {
        return;
    }
    if (m_paneFooter) {
        RemoveChild(m_paneFooter);
    }
    m_paneFooter = footer;
    if (m_paneFooter) {
        AddChild(m_paneFooter);
    }
    RelayoutChildren();
}

void NavigationView::SetAutoSuggestBox(const std::shared_ptr<UIElement>& box) {
    if (m_autoSuggestBox == box) {
        return;
    }
    if (m_autoSuggestBox) {
        RemoveChild(m_autoSuggestBox);
    }
    m_autoSuggestBox = box;
    if (m_autoSuggestBox) {
        AddChild(m_autoSuggestBox);
    }
    RelayoutChildren();
}

void NavigationView::SetIsBackButtonVisible(NavigationViewBackButtonVisible visible) {
    m_backVisible = visible;
    RelayoutChildren();
}

void NavigationView::SetIsBackEnabled(bool enabled) {
    m_backEnabled = enabled;
    if (m_btnBack) {
        m_btnBack->SetIsEnabled(enabled);
    }
}

void NavigationView::SetSelectedItem(NavigationViewItem* item) {
    if (m_selectedItem == item) {
        UpdateSelectionVisuals();
        return;
    }
    NavigationViewItem* previous = m_selectedItem;
    m_selectedItem = item;
    StartSelectionIndicatorAnimation(previous, m_selectedItem);
    UpdateSelectionVisuals();

    if (!m_ignoreSelectionEvent) {
        NavigationViewSelectionChangedEventArgs args;
        args.SelectedItem = m_selectedItem;
        args.IsSettingsSelected = (m_selectedItem != nullptr && m_selectedItem == m_settingsItem.get());
        m_selectionChanged.Invoke(this, args);
    }
    MarkRenderContentDirty();
}

Rect NavigationView::GetIndicatorRectForItem(const NavigationViewItem* item) const {
    if (!item) {
        return Rect();
    }
    const Rect b = item->GetBounds();
    if (b.IsEmpty()) {
        return Rect();
    }
    if (IsTopNavigation()) {
        return Rect(b.x + 8.0f, b.y + b.height - 3.0f, (std::max)(16.0f, b.width - 16.0f), 3.0f);
    }
    return Rect(b.x + 2.0f, b.y + 8.0f, 3.0f, (std::max)(4.0f, b.height - 16.0f));
}

void NavigationView::StartSelectionIndicatorAnimation(NavigationViewItem* from, NavigationViewItem* to) {
    if (!to) {
        m_selectionIndicatorFrom = nullptr;
        m_selectionIndicatorTo = nullptr;
        m_selectionIndicatorAnim.Reset(1.0f);
        return;
    }
    m_selectionIndicatorFrom = from ? from : to;
    m_selectionIndicatorTo = to;
    m_selectionIndicatorAnim.Reset(0.0f);
    m_selectionIndicatorAnim.SetTarget(1.0f);
    EnsureAnimationsScheduled();
}

void NavigationView::SelectByTag(const std::string& tag) {
    std::vector<NavigationViewItemBase*> visible;
    CollectVisibleItems(visible);
    for (auto* base : visible) {
        auto* item = dynamic_cast<NavigationViewItem*>(base);
        if (item && item->GetTag() == tag) {
            SetSelectedItem(item);
            return;
        }
    }
    // Also search collapsed children.
    auto search = [&](auto&& self, const std::shared_ptr<NavigationViewItemBase>& node) -> bool {
        if (auto* item = dynamic_cast<NavigationViewItem*>(node.get())) {
            if (item->GetTag() == tag) {
                SetSelectedItem(item);
                return true;
            }
            for (auto& child : item->MenuItems()) {
                if (self(self, child)) {
                    return true;
                }
            }
        }
        return false;
    };
    for (auto& item : m_menuItems) {
        if (search(search, item)) return;
    }
    for (auto& item : m_footerItems) {
        if (search(search, item)) return;
    }
    if (m_settingsItem && m_settingsItem->GetTag() == tag) {
        SetSelectedItem(m_settingsItem.get());
    }
}

void NavigationView::NotifyItemInvoked(NavigationViewItem* item) {
    if (!item) {
        return;
    }

    NavigationViewItemInvokedEventArgs invokedArgs;
    invokedArgs.InvokedItem = item;
    invokedArgs.IsSettingsInvoked = (item == m_settingsItem.get());
    m_itemInvoked.Invoke(this, invokedArgs);

    if (item->SelectsOnInvoked()) {
        SetSelectedItem(item);
        ClosePaneIfOverlay();
    } else if (item->HasChildren()) {
        // Already toggled expand in item click path when SelectsOnInvoked is false.
    }
}

void NavigationView::ClosePaneIfOverlay() {
    if (IsOverlayMode() && m_isPaneOpen) {
        SetIsPaneOpen(false);
    }
}

bool NavigationView::IsOverlayMode() const {
    // When pane is open in compact/minimal, we want inline behavior:
    // content must give the pane its space (no overlay).
    // When pane is closed, it overlays the content.
    if (m_displayMode == NavigationViewDisplayMode::Minimal
        || m_displayMode == NavigationViewDisplayMode::Compact) {
        return !m_isPaneOpen;
    }
    return false;
}

bool NavigationView::ShouldShowHeader() const {
    if (m_header.empty()) {
        return false;
    }
    if (m_alwaysShowHeader) {
        return true;
    }
    // WinUI: header auto-shown in Minimal.
    return m_displayMode == NavigationViewDisplayMode::Minimal || IsTopNavigation();
}

bool NavigationView::ShouldShowBackButton() const {
    switch (m_backVisible) {
    case NavigationViewBackButtonVisible::Collapsed:
        return false;
    case NavigationViewBackButtonVisible::Visible:
        return true;
    case NavigationViewBackButtonVisible::Auto:
    default:
        // Auto: hide in Top mode (WinUI).
        return !IsTopNavigation();
    }
}

bool NavigationView::IsCompactList() const {
    if (IsTopNavigation()) {
        return false;
    }
    if (m_displayMode == NavigationViewDisplayMode::Expanded) {
        return false;
    }
    // Compact/Minimal closed → icon rail / hidden labels.
    if (m_displayMode == NavigationViewDisplayMode::Compact && !m_isPaneOpen) {
        return true;
    }
    if (m_displayMode == NavigationViewDisplayMode::Minimal && !m_isPaneOpen) {
        return true;
    }
    // Overlay open → full labels.
    return false;
}

float NavigationView::TargetPaneWidth() const {
    if (IsTopNavigation()) {
        return 0.0f;
    }
    switch (m_displayMode) {
    case NavigationViewDisplayMode::Expanded:
        return (std::min)(m_openPaneLength, (std::max)(0.0f, m_bounds.width));
    case NavigationViewDisplayMode::Compact:
        return m_isPaneOpen
            ? (std::min)(m_openPaneLength, (std::max)(0.0f, m_bounds.width))
            : m_compactPaneLength;
    case NavigationViewDisplayMode::Minimal:
    default:
        return m_isPaneOpen
            ? (std::min)(m_openPaneLength, (std::max)(0.0f, m_bounds.width))
            : 0.0f;
    }
}

float NavigationView::EffectivePaneWidth() const {
    if (IsTopNavigation()) {
        return 0.0f;
    }
    if (!UIElement::AreAnimationsEnabled()) {
        return TargetPaneWidth();
    }
    return m_paneWidthAnim.Current();
}

void NavigationView::UpdateAdaptiveLayout(float width) {
    NavigationViewDisplayMode next = m_displayMode;

    switch (m_paneDisplayMode) {
    case NavigationViewPaneDisplayMode::Left:
        next = NavigationViewDisplayMode::Expanded;
        break;
    case NavigationViewPaneDisplayMode::LeftCompact:
        next = NavigationViewDisplayMode::Compact;
        break;
    case NavigationViewPaneDisplayMode::LeftMinimal:
        next = NavigationViewDisplayMode::Minimal;
        break;
    case NavigationViewPaneDisplayMode::Top:
        // Top uses Expanded-like content; display mode still reported as Expanded.
        next = NavigationViewDisplayMode::Expanded;
        break;
    case NavigationViewPaneDisplayMode::Auto:
    default:
        if (width <= m_compactThreshold) {
            next = NavigationViewDisplayMode::Minimal;
        } else if (width < m_expandedThreshold) {
            next = NavigationViewDisplayMode::Compact;
        } else {
            next = NavigationViewDisplayMode::Expanded;
        }
        break;
    }

    ApplyDisplayMode(next, false);

    // Expanded left pane stays open; overlay modes keep IsPaneOpen as user state.
    if (m_paneDisplayMode == NavigationViewPaneDisplayMode::Left
        || (m_paneDisplayMode == NavigationViewPaneDisplayMode::Auto
            && next == NavigationViewDisplayMode::Expanded)) {
        if (!m_isPaneOpen) {
            m_isPaneOpen = true;
        }
    }

    m_paneWidthAnim.SetTarget(TargetPaneWidth());
    UpdateChildCompactFlags();
    if (std::abs(m_paneWidthAnim.Target() - m_paneWidthAnim.Current()) > 0.001f) {
        EnsureAnimationsScheduled();
    }
}

void NavigationView::ApplyDisplayMode(NavigationViewDisplayMode mode, bool forceEvent) {
    if (m_displayMode == mode && !forceEvent) {
        return;
    }
    m_displayMode = mode;
    NavigationViewDisplayModeChangedEventArgs args;
    args.DisplayMode = m_displayMode;
    m_displayModeChanged.Invoke(this, args);
}

void NavigationView::UpdateChildCompactFlags() {
    const bool compact = IsCompactList();
    const bool top = IsTopNavigation();

    auto apply = [&](auto&& self, const std::shared_ptr<NavigationViewItemBase>& node) -> void {
        if (auto* item = dynamic_cast<NavigationViewItem*>(node.get())) {
            item->SetCompact(compact);
            item->SetTopMode(top);
            for (auto& child : item->MenuItems()) {
                self(self, child);
            }
        }
    };
    for (auto& item : m_menuItems) apply(apply, item);
    for (auto& item : m_footerItems) apply(apply, item);
    if (m_settingsItem) {
        m_settingsItem->SetCompact(compact);
        m_settingsItem->SetTopMode(top);
    }
}

void NavigationView::UpdateSelectionVisuals() {
    auto walk = [&](auto&& self, const std::shared_ptr<NavigationViewItemBase>& node,
                    bool& ancestorSelected) -> bool {
        bool childSelected = false;
        if (auto* item = dynamic_cast<NavigationViewItem*>(node.get())) {
            for (auto& child : item->MenuItems()) {
                bool dummy = false;
                if (self(self, child, dummy)) {
                    childSelected = true;
                }
            }
            const bool selected = (item == m_selectedItem);
            item->SetIsSelected(selected);
            item->SetIsChildSelected(childSelected && !selected);
            ancestorSelected = selected || childSelected;
            return ancestorSelected;
        }
        return false;
    };

    for (auto& item : m_menuItems) {
        bool dummy = false;
        walk(walk, item, dummy);
    }
    for (auto& item : m_footerItems) {
        bool dummy = false;
        walk(walk, item, dummy);
    }
    if (m_settingsItem) {
        m_settingsItem->SetIsSelected(m_settingsItem.get() == m_selectedItem);
        m_settingsItem->SetIsChildSelected(false);
    }
}

void NavigationView::CollectVisibleFrom(const std::shared_ptr<NavigationViewItemBase>& item,
                                        std::vector<NavigationViewItemBase*>& out) const {
    if (!item) {
        return;
    }
    out.push_back(item.get());
    if (auto* nvi = dynamic_cast<NavigationViewItem*>(item.get())) {
        if (nvi->IsExpanded()) {
            for (auto& child : nvi->MenuItems()) {
                CollectVisibleFrom(child, out);
            }
        }
    }
}

void NavigationView::CollectVisibleItems(std::vector<NavigationViewItemBase*>& out) const {
    out.clear();
    for (auto& item : m_menuItems) {
        CollectVisibleFrom(item, out);
    }
    for (auto& item : m_footerItems) {
        CollectVisibleFrom(item, out);
    }
    if (m_settingsVisible && m_settingsItem) {
        out.push_back(m_settingsItem.get());
    }
}

Rect NavigationView::GetPaneRect() const {
    if (IsTopNavigation()) {
        return Rect();
    }
    const float w = EffectivePaneWidth();
    return Rect(m_bounds.x, m_bounds.y, w, m_bounds.height);
}

Rect NavigationView::GetTopNavRect() const {
    if (!IsTopNavigation()) {
        return Rect();
    }
    return Rect(m_bounds.x, m_bounds.y, m_bounds.width, kTopNavHeight);
}

Rect NavigationView::GetContentHostRect() const {
    if (IsTopNavigation()) {
        const float top = kTopNavHeight;
        return Rect(m_bounds.x, m_bounds.y + top, m_bounds.width, (std::max)(0.0f, m_bounds.height - top));
    }

    // Inline behavior: when the pane is open, content must be shifted by
    // the effective pane width so it doesn't overlap and blocking clicks.
    float left = 0.0f;
    if (m_displayMode == NavigationViewDisplayMode::Expanded) {
        left = EffectivePaneWidth();
    } else if (m_displayMode == NavigationViewDisplayMode::Compact) {
        left = m_isPaneOpen ? EffectivePaneWidth() : m_compactPaneLength;
    } else { // Minimal
        left = m_isPaneOpen ? EffectivePaneWidth() : 0.0f;
    }
    return Rect(m_bounds.x + left, m_bounds.y,
                (std::max)(0.0f, m_bounds.width - left), m_bounds.height);
}

Rect NavigationView::GetHeaderRect() const {
    if (!ShouldShowHeader()) {
        return Rect();
    }
    const Rect host = GetContentHostRect();
    return Rect(host.x, host.y, host.width, DefaultHeaderHeight);
}

Rect NavigationView::GetContentAreaRect() const {
    const Rect host = GetContentHostRect();
    if (!ShouldShowHeader()) {
        return host;
    }
    return Rect(host.x, host.y + DefaultHeaderHeight,
                host.width, (std::max)(0.0f, host.height - DefaultHeaderHeight));
}

void NavigationView::RelayoutChildren() {
    if (m_bounds.width <= 0.0f || m_bounds.height <= 0.0f) {
        return;
    }

    UpdateAdaptiveLayout(m_bounds.width);

    // Hide entire menu tree first; visible list re-shows what should paint.
    auto hideTree = [&](auto&& self, const std::shared_ptr<NavigationViewItemBase>& node) -> void {
        if (!node) return;
        node->SetVisibility(Visibility::Collapsed);
        if (auto* nvi = dynamic_cast<NavigationViewItem*>(node.get())) {
            for (auto& child : nvi->MenuItems()) {
                self(self, child);
            }
        }
    };
    for (auto& item : m_menuItems) hideTree(hideTree, item);
    for (auto& item : m_footerItems) hideTree(hideTree, item);
    if (m_settingsItem) {
        m_settingsItem->SetVisibility(Visibility::Collapsed);
    }

    const bool showBack = ShouldShowBackButton();
    const bool top = IsTopNavigation();
    const bool compactList = IsCompactList();

    if (m_btnBack) {
        m_btnBack->SetVisibility(showBack ? Visibility::Visible : Visibility::Collapsed);
    }
    if (m_btnToggle) {
        // Toggle hidden in Top mode and Forced Left-Expanded (optional: still show).
        const bool showToggle = !top && m_paneDisplayMode != NavigationViewPaneDisplayMode::Left;
        m_btnToggle->SetVisibility(showToggle ? Visibility::Visible : Visibility::Collapsed);
    }

    if (top) {
        // Horizontal top nav row (no vertical ScrollViewer).
        float x = m_bounds.x + 4.0f;
        const float yRow = m_bounds.y + 4.0f;
        if (showBack && m_btnBack) {
            m_btnBack->Measure(Size(kChromeButton, kChromeButton));
            m_btnBack->Arrange(Rect(x, yRow, kChromeButton, kChromeButton));
            x += kChromeButton + 4.0f;
        }
        if (m_btnToggle) {
            m_btnToggle->Arrange(Rect(0, 0, 0, 0));
        }
        if (m_menuScroll) {
            m_menuScroll->SetVisibility(Visibility::Collapsed);
        }

        // Ensure menu items are direct children for Top hit-test (outside collapsed ScrollViewer).
        SyncMenuHostChildren();
        if (m_menuHost) {
            auto childrenCopy = m_menuHost->GetChildren();
            for (auto& child : childrenCopy) {
                m_menuHost->RemoveChild(child);
                AddChild(child);
            }
        }

        std::vector<NavigationViewItemBase*> visible;
        for (auto& item : m_menuItems) {
            CollectVisibleFrom(item, visible);
        }
        for (auto& item : m_footerItems) {
            CollectVisibleFrom(item, visible);
        }
        if (m_settingsVisible && m_settingsItem) {
            visible.push_back(m_settingsItem.get());
        }

        for (auto* base : visible) {
            const float itemW = (std::max)(80.0f, 40.0f + static_cast<float>(
                dynamic_cast<NavigationViewItem*>(base)
                    ? dynamic_cast<NavigationViewItem*>(base)->GetContent().size() * 8.0f
                    : 40));
            base->SetVisibility(Visibility::Visible);
            base->Measure(Size(itemW, kTopNavHeight - 8.0f));
            base->Arrange(Rect(x, yRow, itemW, kTopNavHeight - 8.0f));
            x += itemW + 4.0f;
        }

        if (m_autoSuggestBox) {
            m_autoSuggestBox->SetVisibility(Visibility::Collapsed);
        }
        if (m_paneFooter) {
            m_paneFooter->SetVisibility(Visibility::Collapsed);
        }
    } else {
        EnsureMenuScroll();
        SyncMenuHostChildren();

        const Rect pane = GetPaneRect();
        float y = pane.y + 4.0f;
        float x = pane.x + 4.0f;

        if (showBack && m_btnBack) {
            m_btnBack->Measure(Size(kChromeButton, kChromeButton));
            m_btnBack->Arrange(Rect(x, y, kChromeButton, kChromeButton));
        }
        if (m_btnToggle && m_btnToggle->GetVisibility() == Visibility::Visible) {
            const float toggleX = showBack ? (x + kChromeButton + 4.0f) : x;
            if (compactList || pane.width < m_compactPaneLength + 8.0f) {
                float ty = y;
                if (showBack) {
                    ty += kChromeButton + 4.0f;
                }
                m_btnToggle->Measure(Size(kChromeButton, kChromeButton));
                m_btnToggle->Arrange(Rect(x, ty, kChromeButton, kChromeButton));
                y = ty + kChromeButton + 8.0f;
            } else {
                m_btnToggle->Measure(Size(kChromeButton, kChromeButton));
                m_btnToggle->Arrange(Rect(toggleX, y, kChromeButton, kChromeButton));
                y += kChromeButton + 8.0f;
            }
        } else if (showBack) {
            y += kChromeButton + 8.0f;
        }

        if (m_autoSuggestBox && !compactList && pane.width > 80.0f) {
            m_autoSuggestBox->SetVisibility(Visibility::Visible);
            const float boxH = 32.0f;
            m_autoSuggestBox->Measure(Size(pane.width - 16.0f, boxH));
            m_autoSuggestBox->Arrange(Rect(pane.x + 8.0f, y, pane.width - 16.0f, boxH));
            y += boxH + 8.0f;
        } else if (m_autoSuggestBox) {
            m_autoSuggestBox->SetVisibility(Visibility::Collapsed);
        }

        const float itemW = (std::max)(36.0f, pane.width - 8.0f);

        // Footer + settings pinned to bottom (outside scroll viewport).
        float footerY = pane.y + pane.height - 4.0f;
        if (m_paneFooter && !compactList && pane.width > 80.0f) {
            m_paneFooter->SetVisibility(Visibility::Visible);
            m_paneFooter->Measure(Size(pane.width - 16.0f, 40.0f));
            const float fh = m_paneFooter->GetDesiredSize().height;
            footerY -= fh;
            m_paneFooter->Arrange(Rect(pane.x + 8.0f, footerY, pane.width - 16.0f, fh));
            footerY -= 4.0f;
        } else if (m_paneFooter) {
            m_paneFooter->SetVisibility(Visibility::Collapsed);
        }

        if (m_settingsVisible && m_settingsItem) {
            m_settingsItem->SetVisibility(pane.width > 0.5f ? Visibility::Visible : Visibility::Collapsed);
            m_settingsItem->Measure(Size(itemW, 40.0f));
            footerY -= m_settingsItem->GetDesiredSize().height;
            m_settingsItem->Arrange(Rect(pane.x + 4.0f, footerY, itemW, m_settingsItem->GetDesiredSize().height));
            footerY -= 2.0f;
        } else if (m_settingsItem) {
            m_settingsItem->SetVisibility(Visibility::Collapsed);
        }

        std::vector<NavigationViewItemBase*> footerVisible;
        for (auto& item : m_footerItems) {
            CollectVisibleFrom(item, footerVisible);
        }
        for (int i = static_cast<int>(footerVisible.size()) - 1; i >= 0; --i) {
            auto* base = footerVisible[static_cast<size_t>(i)];
            base->SetVisibility(pane.width > 0.5f ? Visibility::Visible : Visibility::Collapsed);
            base->Measure(Size(itemW, 40.0f));
            footerY -= base->GetDesiredSize().height;
            base->Arrange(Rect(pane.x + 4.0f, footerY, itemW, base->GetDesiredSize().height));
            footerY -= 2.0f;
        }

        // Menu ScrollViewer fills the remaining pane between chrome and footer.
        // This clips content so scrolled items cannot paint over the pane title.
        const float menuTopY = y;
        const float menuBottomY = (std::max)(menuTopY, footerY - 4.0f);
        const float menuH = (std::max)(0.0f, menuBottomY - menuTopY);
        if (m_menuScroll) {
            m_menuScroll->SetVisibility(pane.width > 0.5f ? Visibility::Visible : Visibility::Collapsed);
            m_menuScroll->Measure(Size(pane.width - 4.0f, menuH));
            m_menuScroll->Arrange(Rect(pane.x + 2.0f, menuTopY, pane.width - 4.0f, menuH));
        }
    }

    // Content + header
    const Rect headerRect = GetHeaderRect();
    const Rect contentRect = GetContentAreaRect();
    if (m_contentAnimating) {
        const float t = std::clamp(m_contentFadeAnim.Current(), 0.0f, 1.0f);
        const float inv = 1.0f - t;
        const float ease = 1.0f - inv * inv * inv;
        if (m_contentNext) {
            m_contentNext->SetOpacity(ease);
            m_contentNext->SetVisibility(Visibility::Visible);
            m_contentNext->Measure(Size(contentRect.width, contentRect.height));
            // Fade only — do NOT slide Arrange by a fractional Y. DrawText snaps
            // to device pixels, so a moving fractional layout origin makes glyphs jitter.
            m_contentNext->Arrange(contentRect);
        }
    } else if (m_content) {
        m_content->SetOpacity(1.0f);
        m_content->SetVisibility(Visibility::Visible);
        m_content->Measure(Size(contentRect.width, contentRect.height));
        m_content->Arrange(contentRect);
    }
    (void)headerRect;
}

Size NavigationView::Measure(Size availableSize) {
    m_desiredSize = availableSize;
    return m_desiredSize;
}

void NavigationView::Arrange(Rect finalRect) {
    SetBounds(finalRect);
    UpdateAdaptiveLayout(finalRect.width);
    m_paneWidthAnim.SetTarget(TargetPaneWidth());
    if (!UIElement::AreAnimationsEnabled()) {
        m_paneWidthAnim.Reset(TargetPaneWidth());
    } else if (std::abs(m_paneWidthAnim.Target() - m_paneWidthAnim.Current()) > 0.001f) {
        EnsureAnimationsScheduled();
    }
    RelayoutChildren();
}

void NavigationView::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const D2D1_COLOR_F paneBg = ResolveThemeColor(GetPaneBackgroundToken(), ThemeTokenId::PaneBackground);
    const D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    const D2D1_COLOR_F textPrimary = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    const D2D1_COLOR_F textSecondary = ResolveThemeColor(GetSecondaryColorToken(), ThemeTokenId::TextSecondary);

    if (IsTopNavigation()) {
        const Rect top = GetTopNavRect();
        ctx.FillRect(top, paneBg);
        ctx.DrawLine(Point(top.x, top.y + top.height),
                     Point(top.x + top.width, top.y + top.height), border, 1.0f);
    } else {
        const Rect pane = GetPaneRect();
        if (pane.width > 0.5f) {
            ctx.FillRect(pane, paneBg);
            // Overlay shadow edge when pane overlays content.
            if (IsOverlayMode() && m_isPaneOpen) {
                ctx.DrawLine(Point(pane.x + pane.width, pane.y),
                             Point(pane.x + pane.width, pane.y + pane.height), border, 1.0f);
            } else if (m_displayMode == NavigationViewDisplayMode::Expanded
                       || m_displayMode == NavigationViewDisplayMode::Compact) {
                ctx.DrawLine(Point(pane.x + pane.width, pane.y),
                             Point(pane.x + pane.width, pane.y + pane.height), border, 1.0f);
            }

            // Pane title next to toggle when list is full-sized.
            if (!IsCompactList() && pane.width > 100.0f && !m_paneTitle.empty()) {
                const bool showBack = ShouldShowBackButton();
                float titleX = pane.x + 8.0f + kChromeButton + 4.0f;
                if (showBack) {
                    titleX += kChromeButton + 4.0f;
                }
                // When toggle is collapsed (Left mode), title sits after back only.
                if (m_paneDisplayMode == NavigationViewPaneDisplayMode::Left) {
                    titleX = pane.x + 8.0f + (showBack ? (kChromeButton + 4.0f) : 0.0f);
                }
                Rect titleRect(titleX, pane.y + 4.0f,
                               (std::max)(0.0f, pane.x + pane.width - titleX - 8.0f), kChromeButton);
                ctx.PushClip(titleRect);
                ctx.DrawText(m_paneTitle, titleRect, textPrimary, "Segoe UI", 14.0f,
                             DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                             DWRITE_FONT_WEIGHT_SEMI_BOLD);
                ctx.PopClip();
            }
        }
    }

    // Content header band (WinUI 52 DIP).
    if (ShouldShowHeader()) {
        const Rect header = GetHeaderRect();
        ctx.DrawText(m_header, Rect(header.x + 24.0f, header.y, header.width - 32.0f, header.height),
                     textPrimary, "Segoe UI", 20.0f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                     DWRITE_FONT_WEIGHT_SEMI_BOLD);
        ctx.DrawLine(Point(header.x, header.y + header.height),
                     Point(header.x + header.width, header.y + header.height),
                     border, 1.0f);
        (void)textSecondary;
    }
}

void NavigationView::OnRenderOverlay(GraphicsContext& ctx) {
    Control::OnRenderOverlay(ctx);

    if (m_selectedItem || m_selectionIndicatorTo) {
        const D2D1_COLOR_F indicator = ResolveThemeColor(GetIndicatorColorToken(), ThemeTokenId::AccentColor);
        const Rect fromRect = GetIndicatorRectForItem(m_selectionIndicatorFrom ? m_selectionIndicatorFrom : m_selectedItem);
        const Rect toRect = GetIndicatorRectForItem(m_selectionIndicatorTo ? m_selectionIndicatorTo : m_selectedItem);
        if (!toRect.IsEmpty()) {
            float t = 1.0f;
            if (m_selectionIndicatorAnim.Current() < 0.999f || m_selectionIndicatorAnim.Target() < 0.999f) {
                t = std::clamp(m_selectionIndicatorAnim.Current(), 0.0f, 1.0f);
            }
            Rect r = toRect;
            if (!fromRect.IsEmpty()) {
                r.x = fromRect.x + (toRect.x - fromRect.x) * t;
                r.y = fromRect.y + (toRect.y - fromRect.y) * t;
                r.width = fromRect.width + (toRect.width - fromRect.width) * t;
                r.height = fromRect.height + (toRect.height - fromRect.height) * t;
            }

            // Clip indicator to menu viewport so it doesn't draw over pane title / footer.
            if (m_menuScroll && !IsTopNavigation()) {
                const Rect clip = m_menuScroll->GetBounds();
                if (!clip.IsEmpty() && !clip.Intersects(r)) {
                    // skip
                } else if (!clip.IsEmpty()) {
                    ctx.PushClip(clip);
                    ctx.FillRoundedRect(r, 1.5f, indicator);
                    ctx.PopClip();
                } else {
                    ctx.FillRoundedRect(r, 1.5f, indicator);
                }
            } else {
                ctx.FillRoundedRect(r, 1.5f, indicator);
            }
        }
    }

    // Entrance transition uses opacity fade only (see RelayoutChildren / OnAnimationTick).
    // Do not draw a semi-opaque windowBackground veil here: on light + Mica it
    // washes out the entire content host (demo + PropertyGrid) until the ~220ms
    // fade completes, which looks like broken theme colors on page load.
}

void NavigationView::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    // Light-dismiss: tap content area closes overlay pane.
    if (IsOverlayMode() && m_isPaneOpen) {
        const Rect pane = GetPaneRect();
        if (!pane.Contains(pt.x, pt.y)) {
            SetIsPaneOpen(false);
        }
    }
}

void NavigationView::OnMouseWheel(float delta) {
    if (IsTopNavigation()) {
        Control::OnMouseWheel(delta);
        return;
    }
    // Scroll the pane menu only when the cursor is over the pane; otherwise let
    // bubbled wheel reach the content ScrollViewer (showcase page, etc.).
    if (m_menuScroll && m_menuScroll->GetVisibility() == Visibility::Visible) {
        POINT screenPt{};
        if (GetCursorPos(&screenPt)) {
            HWND hwnd = WindowFromPoint(screenPt);
            float lx = 0.0f;
            float ly = 0.0f;
            if (hwnd && TryGetCursorClientLogical(hwnd, lx, ly) && GetPaneRect().Contains(lx, ly)) {
                m_menuScroll->OnMouseWheel(delta);
                return;
            }
        }
    }
    Control::OnMouseWheel(delta);
}

bool NavigationView::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    m_paneWidthAnim.SetTarget(TargetPaneWidth());
    const bool widthAnim = m_paneWidthAnim.Tick(dt, AnimationSpec{ 0.55f, 0.01f });
    const bool indicatorAnim = m_selectionIndicatorAnim.Tick(dt, AnimationSpec{ 0.65f, 0.001f });
    const bool contentAnim = m_contentAnimating && m_contentFadeAnim.Tick(dt, AnimationSpec{ 0.22f, 0.001f });

    if (contentAnim) {
        const Rect contentRect = GetContentAreaRect();
        const float t = std::clamp(m_contentFadeAnim.Current(), 0.0f, 1.0f);
        const float inv = 1.0f - t;
        const float ease = 1.0f - inv * inv * inv;
        if (m_contentNext) {
            m_contentNext->SetOpacity(ease);
            // Keep layout origin stable; opacity-only entrance avoids text snap jitter.
            m_contentNext->Arrange(contentRect);
        }
        MarkRenderContentDirty();
    }
    if (m_contentAnimating && !m_contentFadeAnim.IsAnimating(0.001f)) {
        m_content = m_contentNext;
        m_contentNext.reset();
        m_contentAnimating = false;
        m_contentFadeAnim.Reset(1.0f);
        if (m_content) {
            m_content->SetOpacity(1.0f);
        }
        RelayoutChildren();
        EnsureContentZOrder();
        MarkRenderContentDirty();
    }
    if (widthAnim) {
        RelayoutChildren();
        MarkRenderContentDirty();
    }

    const bool stillAnimating = base || widthAnim || indicatorAnim || contentAnim || m_contentAnimating
        || std::abs(m_paneWidthAnim.Target() - m_paneWidthAnim.Current()) > 0.001f
        || m_selectionIndicatorAnim.IsAnimating(0.001f)
        || m_contentFadeAnim.IsAnimating(0.001f);
    if (stillAnimating) {
        EnsureAnimationsScheduled();
    }
    return stillAnimating;
}

bool NavigationView::HasSelfAnimation() const {
    return Control::HasSelfAnimation()
        || std::abs(m_paneWidthAnim.Target() - m_paneWidthAnim.Current()) > 0.001f
        || m_selectionIndicatorAnim.IsAnimating(0.001f)
        || m_contentAnimating
        || m_contentFadeAnim.IsAnimating(0.001f);
}

} // namespace CUI
