#pragma once

#include "Control.h"
#include "../animation/AnimationSystem.h"
#include <memory>
#include <string>

namespace CUI {

enum class FlyoutPlacement {
    Top,
    Bottom,
    Left,
    Right
};

class FlyoutPresenter : public Control {
public:
    FlyoutPresenter();
    virtual ~FlyoutPresenter() = default;

    virtual const char* GetClassName() const override { return "FlyoutPresenter"; }

    void SetContent(std::shared_ptr<UIElement> content);
    std::shared_ptr<UIElement> GetContent() const { return m_content; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;

private:
    std::shared_ptr<UIElement> m_content;
};

class Flyout : public UIElement {
public:
    Flyout();
    explicit Flyout(std::shared_ptr<UIElement> content);
    virtual ~Flyout() = default;

    virtual const char* GetClassName() const override { return "Flyout"; }

    void SetContent(std::shared_ptr<UIElement> content);
    std::shared_ptr<UIElement> GetContent() const { return m_presenter ? m_presenter->GetContent() : nullptr; }

    void SetPlacement(FlyoutPlacement placement) { m_placement = placement; }
    FlyoutPlacement GetPlacement() const { return m_placement; }

    void ShowAt(UIElement* target);
    void ShowAt(Point pt);
    void Hide();
    bool IsOpen() const { return m_isOpen; }

    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* HitTestOverlay(float x, float y) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;

private:
    std::shared_ptr<FlyoutPresenter> m_presenter;
    FlyoutPlacement m_placement = FlyoutPlacement::Bottom;
    bool m_isOpen = false;
    Point m_popupPos{ 0.0f, 0.0f };
    Size m_popupSize{ 220.0f, 140.0f };
    AnimatedScalar m_popupAnim{ 0.0f };
};

} // namespace CUI
