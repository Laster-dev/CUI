#pragma once

#include "Control.h"
#include "../animation/AnimationSystem.h"
#include "../window/PopupHost.h"
#include "../window/BubbleChrome.h"
#include <string>

namespace CUI {

class TeachingTip : public Control, public IPopup {
public:
    TeachingTip();
    virtual ~TeachingTip();

    virtual const char* GetClassName() const override { return "TeachingTip"; }

    void SetTitle(const std::string& title);
    const std::string& GetTitle() const { return m_title; }
    void SetMessage(const std::string& message);
    const std::string& GetMessage() const { return m_message; }
    void SetActionText(const std::string& text);
    const std::string& GetActionText() const { return m_actionText; }
    void SetIsCloseVisible(bool visible);
    bool GetIsCloseVisible() const { return m_closeVisible; }
    void SetIsModal(bool modal);
    bool GetIsModal() const { return m_isModal; }
    void SetPreferredPlacement(BubblePlacement placement);
    BubblePlacement GetPreferredPlacement() const { return m_preferredPlacement; }
    void SetMaxWidth(float width);
    float GetMaxWidth() const { return m_maxWidth; }

    void ShowAround(UIElement* target);
    void Close();
    bool IsOpen() const { return m_isOpen; }

    Event<>& OnAction() { return m_onAction; }
    Event<>& OnClosed() { return m_onClosed; }

    // IPopup
    virtual bool IsPopupOpen() const override { return m_isOpen; }
    virtual Rect GetPopupBounds() const override;
    virtual bool HitDismissExempt(float x, float y) const override;
    virtual UIElement* HitTestPopup(float x, float y) override;
    virtual void RenderPopup(GraphicsContext& ctx) override;
    virtual void OnLightDismiss() override;
    virtual void CollectPopupDirty(Rect& dirtyRect, bool& hasDirty) const override;

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual bool OnKeyDown(int vkCode) override;
    virtual bool AcceptsTabFocus() const override { return m_isOpen; }
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual bool IsModalOverlayOpen() const override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual bool ShouldClipToBounds() const override { return false; }
    virtual void OnNavigatedFrom() override;

private:
    enum class Hotspot : uint8_t { None, Close, Action, Card };

    void Relayout();
    void DirtyPopup();
    Hotspot HitHotspot(float x, float y) const;
    bool SetHotHover(Hotspot hotspot);

    std::string m_title;
    std::string m_message;
    std::string m_actionText;
    bool m_closeVisible = true;
    bool m_isModal = false;
    bool m_isOpen = false;
    bool m_actionPressed = false;
    float m_maxWidth = 320.0f;
    BubblePlacement m_preferredPlacement = BubblePlacement::Auto;
    UIElement* m_anchor = nullptr;
    BubbleLayout m_layout{};
    Rect m_titleRect{};
    Rect m_bodyRect{};
    Rect m_closeRect{};
    Rect m_actionRect{};
    Hotspot m_hotHover = Hotspot::None;
    AnimatedScalar m_popupAnim{ 0.0f };
    Event<> m_onAction;
    Event<> m_onClosed;
};

} // namespace CUI
