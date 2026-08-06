#pragma once
#include "UIElement.h"
#include "Panel.h"
#include "../animation/AnimationSystem.h"
#include <memory>
#include <string>

namespace CUI {

// Self-contained accordion card: native header chrome (no Button child).
class CollapsePanel : public UIElement {
public:
    CollapsePanel();
    explicit CollapsePanel(const std::string& headerText);
    virtual ~CollapsePanel() = default;

    virtual const char* GetClassName() const override { return "CollapsePanel"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    void SetHeader(const std::string& header);
    std::string GetHeader() const { return m_headerText; }

    void SetSubtitle(const std::string& subtitle);
    std::string GetSubtitle() const { return m_subtitleText; }

    void SetExpanded(bool expanded);
    bool IsExpanded() const { return m_isExpanded; }

    void SetContent(std::shared_ptr<UIElement> content);
    std::shared_ptr<UIElement> GetContent() const { return m_content; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual UIElement* HitTest(float x, float y) override;

    virtual void OnMouseEnter() override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnKeyDown(int vkCode) override;
    bool OnAnimationTick() override;
    bool HasSelfAnimation() const override;

    Event<CollapsePanel*, bool>& OnExpandedChanged() { return m_onExpandedChangedEvent; }

private:
    float GetHeaderHeight() const;
    Rect GetHeaderRect() const;
    bool IsPointInHeader(Point pt) const;
    void UpdateContentVisibility();
    void InvalidateParentLayout();
    void SetHeaderHovered(bool hovered);
    void DrawHeader(GraphicsContext& ctx);
    void DrawAnimatedChevron(GraphicsContext& ctx, const Rect& bounds, float progress);

    std::string m_headerText = "折叠面板";
    std::string m_subtitleText;
    bool m_isExpanded = true;
    bool m_headerHovered = false;
    bool m_headerPressed = false;
    AnimatedScalar m_expandAnim{ 1.0f };
    float m_bodyDesiredHeight = 0.0f;
    std::shared_ptr<StackPanel> m_contentHost;
    std::shared_ptr<UIElement> m_content;

    Event<CollapsePanel*, bool> m_onExpandedChangedEvent;
};

} // namespace CUI
