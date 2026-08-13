#pragma once
#include "UIElement.h"
#include "Button.h"
#include "TextBlock.h"
#include "TextBox.h"
#include "../render/RenderLayer.h"
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

    // Optional single-line / multi-line text input (CUI TextBox — not a Win32 dialog).
    void SetInputEnabled(bool enabled, bool multiline = false);
    void SetInputText(const std::string& text);
    std::string GetInputText() const;
    bool IsInputEnabled() const { return m_inputEnabled; }

    void Show(std::function<void(DialogResult)> callback = nullptr);
    void Hide();

    // Force re-raster of the dialog card (caret / selection / text changes).
    void InvalidateCard();

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
    virtual bool NeedsOverlayHitTest() const override { return true; }

    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual void OnThemeChanged() override;

    static void ShowMessageBox(UIElement* root, const std::string& title, const std::string& message,
                               std::function<void(DialogResult)> callback = nullptr);

    // Async CUI prompt. Callback receives DialogResult::Primary and the entered text on OK.
    static void ShowInputBox(
        UIElement* root,
        const std::string& title,
        const std::string& message,
        const std::string& initialText,
        bool multiline,
        std::function<void(DialogResult, const std::string&)> callback);

private:
    void LayoutCardChildren(float scale);

    std::string m_titleText = "Message";
    std::string m_messageText = "";
    std::string m_primaryText = "确定";
    std::string m_secondaryText = "";
    std::string m_closeText = "取消";

    bool m_isOpen = false;
    bool m_inputEnabled = false;
    bool m_inputMultiline = false;
    std::function<void(DialogResult)> m_callback = nullptr;

    std::shared_ptr<TextBlock> m_txtTitle;
    std::shared_ptr<TextBlock> m_txtMessage;
    std::shared_ptr<TextBox> m_inputBox;
    std::shared_ptr<Button> m_btnPrimary;
    std::shared_ptr<Button> m_btnSecondary;
    std::shared_ptr<Button> m_btnClose;

    Rect m_dialogBounds;
    RenderLayer m_cardLayer;
    bool m_cardCacheValid = false;

    // Animation state
    int m_animState = 0; // 0=Closed, 1=Opening, 2=Opened, 3=Closing
    std::chrono::steady_clock::time_point m_animStartTime;
    float m_animProgress = 0.0f; // 0.0f -> 1.0f
};

} // namespace CUI
