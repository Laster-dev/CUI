#pragma once
#include "UIElement.h"
#include "Toast.h"
#include <memory>
#include <string>
#include <vector>

namespace CUI {

/**
 * @brief 计算 Toast 的可堆叠放置区域。
 * 完整客户区减去自绘标题栏带（WindowTitleBar）——Toast 绝不能覆盖标题
 * 文本或右上角的最小化/最大化/关闭按钮。
 */
Rect ComputeToastAreaRect(UIElement* root);

class ToastCenter : public UIElement {
public:
    ToastCenter();
    virtual ~ToastCenter() = default;

    virtual const char* GetClassName() const override { return "ToastCenter"; }
    virtual bool ShouldClipToBounds() const override { return false; }
    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRenderOverlay(GraphicsContext& ctx) override;
    virtual UIElement* OnHitTestOverlay(float x, float y) override;
    virtual bool NeedsOverlayHitTest() const override { return true; }
    virtual bool OnAnimationTick() override;
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;

    std::shared_ptr<Toast> AddToast(const std::shared_ptr<Toast>& toast);
    std::shared_ptr<Toast> ShowToast(const std::string& title,
        const std::string& message,
        ToastType type = ToastType::Info,
        ToastCorner corner = ToastCorner::BottomRight,
        int durationMs = 2500);
    std::shared_ptr<Toast> ShowToast(const std::string& title,
        const std::string& message,
        ToastCorner corner,
        int durationMs = 2500);
    std::shared_ptr<Toast> ShowFromTemplate(const UIElement* toastTemplate,
        const std::string& titleOverride = "",
        const std::string& messageOverride = "");
    void DismissAll();
    size_t GetActiveCount() const;
    void NotifyToastChanged();

    static std::shared_ptr<ToastCenter> Ensure(UIElement* root);
    static std::shared_ptr<Toast> Show(UIElement* root,
        const std::string& title,
        const std::string& message,
        ToastType type = ToastType::Info,
        ToastCorner corner = ToastCorner::BottomRight,
        int durationMs = 2500);
    static std::shared_ptr<Toast> Show(UIElement* root,
        const std::string& title,
        const std::string& message,
        ToastCorner corner,
        int durationMs = 2500);

private:
    void Compact();
    void ScheduleAutoCloseWake();
    Rect GetWindowRect() const;
    Rect GetToastAreaRect() const;

    std::vector<std::shared_ptr<Toast>> m_toasts;
};

} // namespace CUI
