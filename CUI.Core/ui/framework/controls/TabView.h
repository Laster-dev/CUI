#pragma once
#include "UIElement.h"
#include "Button.h"
#include "Panel.h"
#include "../animation/AnimationSystem.h"
#include "../render/DirtyRegion.h"
#include "../render/RenderLayer.h"
#include <vector>
#include <string>
#include <memory>

namespace CUI {

struct TabViewItem {
    std::string title;
    std::string icon;
    std::shared_ptr<UIElement> content;
    bool isClosable = true;
    AnimatedScalar accentAnim{};
};

class TabView : public UIElement {
public:
    TabView();
    virtual ~TabView() = default;

    virtual const char* GetClassName() const override { return "TabView"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;

    void AddTab(const std::string& title, std::shared_ptr<UIElement> content, const std::string& icon = "", bool isClosable = true);
    void RemoveTab(int index);
    void SetSelectedIndex(int index);
    int GetSelectedIndex() const { return m_selectedIndex; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool AcceptsTabFocus() const override { return true; }
    virtual bool OnKeyDown(int vkCode) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual UIElement* HitTest(float x, float y) override;
    virtual void SyncRenderState() override;
    virtual void CollectRenderDirtyRegion(DirtyRegion& dirtyRegion, bool consume = true) override;
    virtual void OnThemeChanged() override;

    Event<TabView*, int>& OnSelectionChanged() { return m_selectionChangedEvent; }
    Event<TabView*, int>& OnTabClosed() { return m_tabClosedEvent; }

    float GetMinTabWidth() const { return m_minTabWidth; }
    void SetMinTabWidth(float w) {
        m_minTabWidth = w;
        MarkRenderContentDirty();
    }

    float GetMaxTabWidth() const { return m_maxTabWidth; }
    void SetMaxTabWidth(float w) {
        m_maxTabWidth = w;
        MarkRenderContentDirty();
    }

private:
    bool IsPointInHeader(float x, float y) const;
    void ScrollHeaderByWheel(float delta);
    float GetHeaderHeight() const;
    float MeasureTabWidth(GraphicsContext& ctx, const TabViewItem& tab) const;
    float GetTotalTabsWidth(GraphicsContext& ctx) const;
    void EnsureSelectedTabVisible();
    Rect GetHeaderRect() const;
    Rect GetContentRect() const;
    void MarkHeaderDirty();
    void MarkContentDirty();
    void RenderHeaderLayer(GraphicsContext& ctx);
    void RenderHeaderContents(GraphicsContext& ctx);
    void RenderContentLayer(GraphicsContext& ctx);
    UIElement* GetSelectedContent() const;

    std::vector<TabViewItem> m_tabs;
    float m_minTabWidth = 80.0f;
    float m_maxTabWidth = 260.0f;
    int m_selectedIndex = 0;
    int m_hoveredCloseIndex = -1;
    AnimatedScalar m_scrollOffsetXAnim{};
    float m_scrollTargetX = 0.0f;
    RenderLayer m_headerLayer;
    RenderLayer m_contentLayer;
    DirtyRegion m_headerDirty;
    DirtyRegion m_contentDirty;

    Event<TabView*, int> m_selectionChangedEvent;
    Event<TabView*, int> m_tabClosedEvent;
};

} // namespace CUI
