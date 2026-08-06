#pragma once
#include "Control.h"
#include "Button.h"
#include "NavigationViewItem.h"
#include "ScrollViewer.h"
#include "Panel.h"
#include "../animation/AnimationSystem.h"
#include <memory>
#include <string>
#include <vector>

namespace CUI {

// WinUI NavigationViewPaneDisplayMode — what the app configures.
enum class NavigationViewPaneDisplayMode {
    Auto = 0,
    Left,
    Top,
    LeftCompact,
    LeftMinimal
};

// WinUI NavigationViewDisplayMode — read-only runtime layout state.
enum class NavigationViewDisplayMode {
    Minimal = 0,
    Compact,
    Expanded
};

enum class NavigationViewBackButtonVisible {
    Collapsed = 0,
    Visible,
    Auto
};

struct NavigationViewItemInvokedEventArgs {
    NavigationViewItem* InvokedItem = nullptr;
    bool IsSettingsInvoked = false;
};

struct NavigationViewSelectionChangedEventArgs {
    NavigationViewItem* SelectedItem = nullptr;
    bool IsSettingsSelected = false;
};

struct NavigationViewDisplayModeChangedEventArgs {
    NavigationViewDisplayMode DisplayMode = NavigationViewDisplayMode::Expanded;
};

// WinUI-faithful NavigationView: PaneDisplayMode ≠ DisplayMode ≠ IsPaneOpen.
class NavigationView : public Control {
public:
    static constexpr float DefaultOpenPaneLength = 320.0f;
    static constexpr float DefaultCompactPaneLength = 48.0f;
    static constexpr float DefaultCompactModeThresholdWidth = 640.0f;
    static constexpr float DefaultExpandedModeThresholdWidth = 1008.0f;
    static constexpr float DefaultHeaderHeight = 52.0f;

    NavigationView();
    ~NavigationView() override = default;

    const char* GetClassName() const override { return "NavigationView"; }
    std::vector<PropertyMeta> GetPropertyMetas() const override;

    // --- Pane mode (WinUI) ---
    void SetPaneDisplayMode(NavigationViewPaneDisplayMode mode);
    NavigationViewPaneDisplayMode GetPaneDisplayMode() const { return m_paneDisplayMode; }

    NavigationViewDisplayMode GetDisplayMode() const { return m_displayMode; }

    void SetIsPaneOpen(bool open);
    bool IsPaneOpen() const { return m_isPaneOpen; }
    void TogglePane();

    void SetOpenPaneLength(float length);
    float GetOpenPaneLength() const { return m_openPaneLength; }

    void SetCompactPaneLength(float length);
    float GetCompactPaneLength() const { return m_compactPaneLength; }

    void SetCompactModeThresholdWidth(float w) { m_compactThreshold = w; }
    float GetCompactModeThresholdWidth() const { return m_compactThreshold; }

    void SetExpandedModeThresholdWidth(float w) { m_expandedThreshold = w; }
    float GetExpandedModeThresholdWidth() const { return m_expandedThreshold; }

    // --- Menu collections ---
    void AddMenuItem(const std::shared_ptr<NavigationViewItemBase>& item);
    void ClearMenuItems();
    const std::vector<std::shared_ptr<NavigationViewItemBase>>& MenuItems() const { return m_menuItems; }

    void AddFooterMenuItem(const std::shared_ptr<NavigationViewItemBase>& item);
    void ClearFooterMenuItems();
    const std::vector<std::shared_ptr<NavigationViewItemBase>>& FooterMenuItems() const { return m_footerItems; }

    // --- Settings ---
    void SetIsSettingsVisible(bool visible);
    bool IsSettingsVisible() const { return m_settingsVisible; }
    NavigationViewItem* SettingsItem() const { return m_settingsItem.get(); }

    // --- Selection ---
    void SetSelectedItem(NavigationViewItem* item);
    NavigationViewItem* GetSelectedItem() const { return m_selectedItem; }
    void SelectByTag(const std::string& tag);

    // --- Content / header / pane chrome ---
    void SetContent(const std::shared_ptr<UIElement>& content);
    std::shared_ptr<UIElement> GetContent() const { return m_content; }

    void SetHeader(const std::string& header);
    const std::string& GetHeader() const { return m_header; }

    void SetAlwaysShowHeader(bool always);
    bool AlwaysShowHeader() const { return m_alwaysShowHeader; }

    void SetPaneTitle(const std::string& title);
    const std::string& GetPaneTitle() const { return m_paneTitle; }

    void SetPaneFooter(const std::shared_ptr<UIElement>& footer);
    std::shared_ptr<UIElement> GetPaneFooter() const { return m_paneFooter; }

    void SetAutoSuggestBox(const std::shared_ptr<UIElement>& box);
    std::shared_ptr<UIElement> GetAutoSuggestBox() const { return m_autoSuggestBox; }

    // --- Back button ---
    void SetIsBackButtonVisible(NavigationViewBackButtonVisible visible);
    NavigationViewBackButtonVisible GetIsBackButtonVisible() const { return m_backVisible; }

    void SetIsBackEnabled(bool enabled);
    bool IsBackEnabled() const { return m_backEnabled; }

    // --- Layout / render ---
    Size Measure(Size availableSize) override;
    void Arrange(Rect finalRect) override;
    void OnRender(GraphicsContext& ctx) override;
    void OnRenderOverlay(GraphicsContext& ctx) override;
    void OnMouseDown(Point pt) override;
    void OnMouseWheel(float delta) override;
    bool OnAnimationTick() override;
    bool HasSelfAnimation() const override;
    void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;

    // --- Events (WinUI order: ItemInvoked then SelectionChanged) ---
    Event<NavigationView*, const NavigationViewItemInvokedEventArgs&>& OnItemInvoked() { return m_itemInvoked; }
    Event<NavigationView*, const NavigationViewSelectionChangedEventArgs&>& OnSelectionChanged() { return m_selectionChanged; }
    Event<NavigationView*>& OnBackRequested() { return m_backRequested; }
    Event<NavigationView*, const NavigationViewDisplayModeChangedEventArgs&>& OnDisplayModeChanged() { return m_displayModeChanged; }
    Event<NavigationView*>& OnPaneOpened() { return m_paneOpened; }
    Event<NavigationView*>& OnPaneClosed() { return m_paneClosed; }

    // Called by NavigationViewItem when invoked.
    void NotifyItemInvoked(NavigationViewItem* item);

private:
    Rect GetIndicatorRectForItem(const NavigationViewItem* item) const;
    void StartSelectionIndicatorAnimation(NavigationViewItem* from, NavigationViewItem* to);

    void BuildChrome();
    void EnsureMenuScroll();
    void EnsureSettingsItem();
    void WireItem(const std::shared_ptr<NavigationViewItemBase>& item, bool intoMenuScroll);
    void CollectVisibleItems(std::vector<NavigationViewItemBase*>& out) const;
    void CollectVisibleFrom(const std::shared_ptr<NavigationViewItemBase>& item,
                            std::vector<NavigationViewItemBase*>& out) const;
    void SyncMenuHostChildren();
    void EnsureContentZOrder();
    void EnsureAnimationsScheduled();

    void UpdateAdaptiveLayout(float width);
    void ApplyDisplayMode(NavigationViewDisplayMode mode, bool forceEvent);
    float TargetPaneWidth() const;
    float EffectivePaneWidth() const;
    bool IsTopNavigation() const { return m_paneDisplayMode == NavigationViewPaneDisplayMode::Top; }
    bool IsOverlayMode() const;
    bool ShouldShowHeader() const;
    bool ShouldShowBackButton() const;
    bool IsCompactList() const;

    void UpdateChildCompactFlags();
    void RelayoutChildren();
    void UpdateSelectionVisuals();
    void ClosePaneIfOverlay();

    // Hierarchy: keep ancestors expanded when a child is selected; on manual
    // collapse of an ancestor, move selection up to that menu item.
    bool ContainsDescendant(const NavigationViewItem* ancestor, const NavigationViewItem* candidate) const;
    void ExpandAncestorsOf(NavigationViewItem* item);
    void OnItemExpandChanged(NavigationViewItem* folder);

    Rect GetPaneRect() const;
    Rect GetContentHostRect() const;
    Rect GetHeaderRect() const;
    Rect GetContentAreaRect() const;
    Rect GetTopNavRect() const;

    NavigationViewPaneDisplayMode m_paneDisplayMode = NavigationViewPaneDisplayMode::Auto;
    NavigationViewDisplayMode m_displayMode = NavigationViewDisplayMode::Expanded;
    bool m_isPaneOpen = true;

    float m_openPaneLength = DefaultOpenPaneLength;
    float m_compactPaneLength = DefaultCompactPaneLength;
    float m_compactThreshold = DefaultCompactModeThresholdWidth;
    float m_expandedThreshold = DefaultExpandedModeThresholdWidth;

    std::vector<std::shared_ptr<NavigationViewItemBase>> m_menuItems;
    std::vector<std::shared_ptr<NavigationViewItemBase>> m_footerItems;
    std::shared_ptr<NavigationViewItem> m_settingsItem;
    bool m_settingsVisible = true;

    NavigationViewItem* m_selectedItem = nullptr;
    std::shared_ptr<UIElement> m_content;
    std::shared_ptr<UIElement> m_paneFooter;
    std::shared_ptr<UIElement> m_autoSuggestBox;

    std::string m_header;
    std::string m_paneTitle = "Navigation";
    bool m_alwaysShowHeader = false;

    NavigationViewBackButtonVisible m_backVisible = NavigationViewBackButtonVisible::Auto;
    bool m_backEnabled = true;

    std::shared_ptr<Button> m_btnBack;
    std::shared_ptr<Button> m_btnToggle;
    std::shared_ptr<ScrollViewer> m_menuScroll;
    std::shared_ptr<StackPanel> m_menuHost;

    AnimatedScalar m_paneWidthAnim{ DefaultOpenPaneLength };
    bool m_ignoreSelectionEvent = false;

    // Animated selection indicator ("blue bar") that slides between items.
    AnimatedScalar m_selectionIndicatorAnim{ 1.0f };
    NavigationViewItem* m_selectionIndicatorFrom = nullptr;
    NavigationViewItem* m_selectionIndicatorTo = nullptr;

    // Content transition animation (slide incoming page).
    bool m_contentAnimating = false;
    AnimatedScalar m_contentFadeAnim{ 1.0f };
    std::shared_ptr<UIElement> m_contentNext;

    Event<NavigationView*, const NavigationViewItemInvokedEventArgs&> m_itemInvoked;
    Event<NavigationView*, const NavigationViewSelectionChangedEventArgs&> m_selectionChanged;
    Event<NavigationView*> m_backRequested;
    Event<NavigationView*, const NavigationViewDisplayModeChangedEventArgs&> m_displayModeChanged;
    Event<NavigationView*> m_paneOpened;
    Event<NavigationView*> m_paneClosed;
};

} // namespace CUI
