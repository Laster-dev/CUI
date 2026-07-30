#pragma once
#include "UIElement.h"
#include "Button.h"
#include "Panel.h"
#include <vector>
#include <string>
#include <memory>

namespace CUI {

struct TabViewItem {
    std::string title;
    std::string icon;
    std::shared_ptr<UIElement> content;
    bool isClosable = true;
    float accentProgress = 0.0f;
};

class TabView : public UIElement {
public:
    TabView();
    virtual ~TabView() = default;

    virtual const char* GetClassName() const override { return "TabView"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    void AddTab(const std::string& title, std::shared_ptr<UIElement> content, const std::string& icon = "", bool isClosable = true);
    void RemoveTab(int index);
    void SetSelectedIndex(int index);
    int GetSelectedIndex() const { return m_selectedIndex; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual UIElement* HitTest(float x, float y) override;

    Event<TabView*, int>& OnSelectionChanged() { return m_selectionChangedEvent; }
    Event<TabView*, int>& OnTabClosed() { return m_tabClosedEvent; }

private:
    std::vector<TabViewItem> m_tabs;
    int m_selectedIndex = 0;
    int m_hoveredCloseIndex = -1;
    float m_scrollOffsetX = 0.0f;
    float m_scrollTargetX = 0.0f;

    Event<TabView*, int> m_selectionChangedEvent;
    Event<TabView*, int> m_tabClosedEvent;

    bool IsPointInHeader(float x, float y) const;
    void ScrollHeaderByWheel(float delta);
    float GetHeaderHeight() const;
    float MeasureTabWidth(GraphicsContext& ctx, const TabViewItem& tab) const;
    float GetTotalTabsWidth(GraphicsContext& ctx) const;
    void EnsureSelectedTabVisible();
};

} // namespace CUI
