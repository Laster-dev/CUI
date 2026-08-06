#pragma once
#include "UIElement.h"
#include "Button.h"
#include "TextBlock.h"
#include <functional>
#include <string>
#include <memory>
#include <chrono>

namespace CUI {

enum class DialogResult {
    None,
    Primary,
    Secondary,
    Cancel
};

class ContentDialog : public UIElement {
public:
    ContentDialog();
    virtual ~ContentDialog() = default;

    virtual const char* GetClassName() const override { return "ContentDialog"; }

    void SetTitle(const std::string& title);
    void SetMessage(const std::string& message);
    void SetPrimaryButtonText(const std::string& text);
    void SetSecondaryButtonText(const std::string& text);
    void SetCloseButtonText(const std::string& text);

    void Show(std::function<void(DialogResult)> callback = nullptr);
    void Hide();

    bool IsOpen() const { return m_isOpen; }
    virtual bool IsModalOverlayOpen() const override { return m_isOpen; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    // Overlay-only: never paint dialog chrome/children into the scene layer.
    virtual void Render(GraphicsContext& ctx) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* HitTestOverlay(float x, float y) override;
    virtual UIElement* OnHitTestOverlay(float x, float y) override;

    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;

    static void ShowMessageBox(UIElement* root, const std::string& title, const std::string& message, std::function<void(DialogResult)> callback = nullptr);

private:
    std::string m_titleText = "Message";
    std::string m_messageText = "";
    std::string m_primaryText = "OK";
    std::string m_secondaryText = "";
    std::string m_closeText = "Cancel";

    bool m_isOpen = false;
    std::function<void(DialogResult)> m_callback = nullptr;

    std::shared_ptr<TextBlock> m_txtTitle;
    std::shared_ptr<TextBlock> m_txtMessage;
    std::shared_ptr<Button> m_btnPrimary;
    std::shared_ptr<Button> m_btnSecondary;
    std::shared_ptr<Button> m_btnClose;

    Rect m_dialogBounds;

    // Animation state
    int m_animState = 0; // 0=Closed, 1=Opening, 2=Opened, 3=Closing
    std::chrono::steady_clock::time_point m_animStartTime;
    float m_animProgress = 0.0f; // 0.0f -> 1.0f
};

} // namespace CUI
