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
};

class TabView : public UIElement {
public:
    TabView();
    virtual ~TabView() = default;

    virtual const char* GetClassName() const override { return "TabView"; }

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

    Event<TabView*, int>& OnSelectionChanged() { return m_selectionChangedEvent; }
    Event<TabView*, int>& OnTabClosed() { return m_tabClosedEvent; }

private:
    std::vector<TabViewItem> m_tabs;
    int m_selectedIndex = 0;
    int m_hoveredCloseIndex = -1;
    float m_scrollOffsetX = 0.0f;

    Event<TabView*, int> m_selectionChangedEvent;
    Event<TabView*, int> m_tabClosedEvent;
};

} // namespace CUI
