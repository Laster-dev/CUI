#pragma once
#include "UIElement.h"
#include "TextBlock.h"
#include <chrono>
#include <memory>
#include <string>

namespace CUI {

enum class ToastCorner {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

class Toast : public UIElement {
public:
    Toast();
    virtual ~Toast() = default;

    virtual const char* GetClassName() const override { return "Toast"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    void SetTitle(const std::string& title);
    void SetMessage(const std::string& message);
    void SetCorner(ToastCorner corner);
    void SetDurationMs(int durationMs);
    void SetWidth(float width);
    void SetAutoClose(bool enabled);
    void SetBackground(const std::string& color);
    void SetAccent(const std::string& color);
    void SetTitleColor(const std::string& color);
    void SetMessageColor(const std::string& color);
    void SetOffsetX(float offsetX);
    void SetOffsetY(float offsetY);
    void SetSpacing(float spacing);
    void SetCloseable(bool closeable);

    // Copy XML/runtime properties from another element (e.g. <Toast> template).
    void ApplyFrom(const UIElement* source);

    void Show();
    void Hide();
    bool IsOpen() const { return m_state == 1 || m_state == 2 || m_state == 4; }
    bool IsAlive() const { return m_state != 3; }
    bool IsAnimating() const { return IsAlive(); }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* OnHitTestOverlay(float x, float y) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnMouseEnter() override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;

    Rect GetRenderBounds() const { return m_renderBounds; }
    float GetCurrentOpacity() const { return m_currentOpacity; }
    float GetCurrentSlideX() const { return m_currentSlideX; }
    float GetCurrentSlideY() const { return m_currentSlideY; }
    float GetSpacing() const { return m_spacing; }
    ToastCorner GetCorner() const;

    static std::shared_ptr<Toast> Show(UIElement* root,
        const std::string& title,
        const std::string& message,
        ToastCorner corner = ToastCorner::BottomRight,
        int durationMs = 2500);

    void RenderContent(GraphicsContext& ctx, const Rect& bounds, float opacity, float slideX, float slideY);
    Rect CalculateBounds(const Rect& windowRect, int stackIndex = 0) const;

private:
    void UpdateTextElements();
    void SyncMembersFromProperties();
    Rect GetCloseButtonRect() const;
    static D2D1_COLOR_F ColorWithAlpha(const std::string& color, float alpha);
    static ToastCorner ParseCorner(const std::string& corner, ToastCorner fallback = ToastCorner::BottomRight);
    static const char* CornerToString(ToastCorner corner);

    std::string m_titleText = "\xe6\x8f\x90\xe7\xa4\xba";
    std::string m_messageText = "";
    ToastCorner m_corner = ToastCorner::BottomRight;
    int m_durationMs = 2500;
    bool m_autoClose = true;
    bool m_closeable = true;
    bool m_isHovering = false;
    int m_state = 3; // 1 entering, 2 shown, 3 closed, 4 exiting
    std::chrono::steady_clock::time_point m_stateTime{};
    float m_width = 320.0f;
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
    float m_spacing = 12.0f;

    std::string m_background = "#2D2D30";
    std::string m_accent = "#007ACC";
    std::string m_titleColor = "#FFFFFF";
    std::string m_messageColor = "#E5E5E5";

    std::shared_ptr<TextBlock> m_txtTitle;
    std::shared_ptr<TextBlock> m_txtMessage;

    Rect m_boundsCache;
    Rect m_renderBounds;
    Rect m_closeBtnBounds;
    float m_currentOpacity = 0.0f;
    float m_currentSlideX = 0.0f;
    float m_currentSlideY = 0.0f;
};

} // namespace CUI
