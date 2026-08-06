#pragma once
#include "UIElement.h"
#include "Button.h"
#include "Panel.h"
#include "../animation/AnimationSystem.h"
#include <memory>
#include <string>

namespace CUI {

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
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnKeyDown(int vkCode) override;
    bool OnAnimationTick() override;
    bool HasSelfAnimation() const override;

    Event<CollapsePanel*, bool>& OnExpandedChanged() { return m_onExpandedChangedEvent; }

private:
    void UpdateContentVisibility();
    void InvalidateParentLayout();
    void SyncHeaderChrome();
    void DrawAnimatedChevron(GraphicsContext& ctx, const Rect& bounds, float progress);

    std::string m_headerText = "折叠面板";
    std::string m_subtitleText;
    bool m_isExpanded = true;
    AnimatedScalar m_expandAnim{ 1.0f };
    float m_bodyDesiredHeight = 0.0f;
    std::shared_ptr<Button> m_headerButton;
    std::shared_ptr<StackPanel> m_contentHost;
    std::shared_ptr<UIElement> m_content;

    Event<CollapsePanel*, bool> m_onExpandedChangedEvent;
};

} // namespace CUI
