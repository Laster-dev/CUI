#pragma once

#include "Control.h"
#include "Button.h"
#include "../animation/AnimationSystem.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace CUI {

enum class NavigationViewPaneDisplayMode {
    Expanded,
    Compact
};

struct NavigationViewItemData {
    std::string tag;
    std::string content;
    std::string icon;
    std::shared_ptr<UIElement> page;
    Rect bounds{};
};

class NavigationView : public Control {
public:
    NavigationView();
    virtual ~NavigationView() = default;

    virtual const char* GetClassName() const override { return "NavigationView"; }

    void AddItem(const std::string& tag, const std::string& content, const std::string& icon, std::shared_ptr<UIElement> page);
    
    void SetPaneDisplayMode(NavigationViewPaneDisplayMode mode);
    NavigationViewPaneDisplayMode GetPaneDisplayMode() const { return m_paneDisplayMode; }

    void SetAutoCollapse(bool enable) { m_autoCollapse = enable; }
    bool IsAutoCollapseEnabled() const { return m_autoCollapse; }

    void SetPaneOpen(bool open);
    bool IsPaneOpen() const { return m_paneDisplayMode == NavigationViewPaneDisplayMode::Expanded; }
    void TogglePane();

    void SetHeader(const std::string& headerText) { m_headerText = headerText; }
    std::string GetHeader() const { return m_headerText; }

    void SelectItemByTag(const std::string& tag);
    int GetSelectedIndex() const { return m_selectedIndex; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

    Event<NavigationView*, int>& OnSelectionChanged() { return m_selectionChangedEvent; }
    Event<NavigationView*, bool>& OnBackRequested() { return m_backRequestedEvent; }

private:
    void UpdateLayoutRects();

    std::vector<NavigationViewItemData> m_items;
    NavigationViewPaneDisplayMode m_paneDisplayMode = NavigationViewPaneDisplayMode::Expanded;
    bool m_autoCollapse = false;
    int m_selectedIndex = 0;
    int m_hoveredIndex = -1;
    std::string m_headerText = "Navigation";
    
    std::shared_ptr<Button> m_btnBack;
    std::shared_ptr<Button> m_btnTogglePane;

    AnimatedScalar m_paneWidthAnim{ 200.0f };
    AnimatedScalar m_indicatorAnim{ 0.0f };
    Event<NavigationView*, int> m_selectionChangedEvent;
    Event<NavigationView*, bool> m_backRequestedEvent;
};

} // namespace CUI
