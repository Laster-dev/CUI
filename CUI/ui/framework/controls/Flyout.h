#pragma once

#include "Control.h"
#include "../animation/AnimationSystem.h"
#include "../window/PopupHost.h"
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
    virtual void OnRender(GraphicsContext& ctx) override;

private:
    std::shared_ptr<UIElement> m_content;
};

// Overlay popup host — Measure/Arrange are 0x0 so it does not participate in flow layout.
class Flyout : public UIElement, public IPopup {
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

    // IPopup
    virtual bool IsPopupOpen() const override { return m_isOpen; }
    virtual Rect GetPopupBounds() const override;
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override { return HitTestOverlay(x, y); }
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override { Hide(); }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* HitTestOverlay(float x, float y) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual bool ShouldClipToBounds() const override { return false; }

private:
    std::shared_ptr<FlyoutPresenter> m_presenter;
    FlyoutPlacement m_placement = FlyoutPlacement::Bottom;
    bool m_isOpen = false;
    UIElement* m_anchor = nullptr;
    Point m_popupPos{ 0.0f, 0.0f };
    Size m_popupSize{ 220.0f, 140.0f };
    AnimatedScalar m_popupAnim{ 0.0f };
};

} // namespace CUI
