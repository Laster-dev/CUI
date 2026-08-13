#pragma once
#include "Control.h"

namespace CUI {

enum class ExpandDirection {
    Down = 0,
    Up = 1
};

class Expander : public Control {
public:
    Expander();
    explicit Expander(const std::string& headerText);
    virtual ~Expander() = default;

    virtual const char* GetClassName() const override { return "Expander"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual UIElement* HitTest(float x, float y) override;

    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnThemeChanged() override;

    const std::string& GetHeader() const { return m_header; }
    void SetHeader(const std::string& header);

    const std::string& GetSubtitle() const { return m_subtitle; }
    void SetSubtitle(const std::string& subtitle);

    bool IsExpanded() const { return m_isExpanded; }
    void SetIsExpanded(bool expanded);
    void SetExpanded(bool expanded) { SetIsExpanded(expanded); }

    ExpandDirection GetExpandDirection() const { return m_expandDirection; }
    void SetExpandDirection(ExpandDirection direction);

    void SetContent(std::shared_ptr<UIElement> content);
    std::shared_ptr<UIElement> GetContent() const { return m_content; }

    Event<Expander*>& OnExpanding() { return m_onExpanding; }
    Event<Expander*>& OnCollapsed() { return m_onCollapsed; }
    Event<Expander*, bool>& OnExpandedChanged() { return m_onExpandedChanged; }

private:
    static constexpr float kHeaderMinHeight = 48.0f;
    static constexpr float kHeaderHorizontalPadding = 16.0f;
    static constexpr float kHeaderVerticalPadding = 12.0f;
    static constexpr float kBodyPadding = 16.0f;
    static constexpr float kChevronHitSize = 28.0f;
    static constexpr float kChevronGlyphSize = 14.0f;
    static constexpr float kCornerRadius = 8.0f;

    float MeasureHeaderHeight(float width) const;
    float MeasureBodyHeight(float width);
    float GetHeaderTextRight() const;
    float GetExpandProgress() const;
    float GetVisibleBodyHeight() const;
    Rect GetHeaderRect() const;
    Rect GetBodyRect() const;
    Rect GetBodyClipRect() const;
    Rect GetChevronRect() const;
    bool IsPointInHeader(Point pt) const;
    void UpdateContentVisibility();
    void InvalidateExpanderLayout();
    void InvalidateExpanderVisual();

    std::string m_header{ "Expander" };
    std::string m_subtitle;
    bool m_isExpanded = false;
    bool m_headerHovered = false;
    bool m_headerPressed = false;
    ExpandDirection m_expandDirection = ExpandDirection::Down;

    std::shared_ptr<UIElement> m_content;
    float m_headerHeight = kHeaderMinHeight;
    float m_measuredBodyHeight = 0.0f;

    AnimatedScalar m_expandAnim{ 0.0f };

    Event<Expander*> m_onExpanding;
    Event<Expander*> m_onCollapsed;
    Event<Expander*, bool> m_onExpandedChanged;
};

using CollapsePanel = Expander;

} // namespace CUI
