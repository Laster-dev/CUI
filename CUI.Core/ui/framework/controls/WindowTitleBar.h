#pragma once

#include "Control.h"
#include "MenuBar.h"
#include "../animation/AnimationSystem.h"
#include "../window/IWindowChrome.h"

#include <wrl/client.h>
#include <d2d1_1.h>

namespace CUI {

class WindowTitleBar : public Control, public IWindowChrome {
public:
    WindowTitleBar();
    virtual ~WindowTitleBar();

    virtual const char* GetClassName() const override { return "WindowTitleBar"; }
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnThemeChanged() override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnBlur() override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual UIElement* HitTest(float x, float y) override;

    virtual UIElement* GetChromeElement() override { return this; }
    virtual const UIElement* GetChromeElement() const override { return this; }
    virtual bool IsInteractiveHit(float x, float y) const override;
    virtual bool IsCaptionDragHit(float x, float y, UIElement* treeHit) const override;
    virtual LRESULT HitTestNonClient(float x, float y) const override;
    virtual bool ConsumeChromeDirty() override;
    virtual void NotifyNonClientMouseMove(float x, float y) override;
    virtual void NotifyNonClientMouseLeave() override;

    MenuBar& GetMenuBar() { return m_menuBar; }
    const MenuBar& GetMenuBar() const { return m_menuBar; }

    void SetTitle(const std::string& title);
    const std::string& GetTitle() const { return m_title; }

    void SetIconText(const std::string& iconText);
    const std::string& GetIconText() const { return m_iconText; }

    // Native HICON drawn in the title-bar badge (e.g. regedit.exe).
    // takeOwnership=true => WindowTitleBar destroys the icon on replace/dtor.
    void SetNativeIcon(HICON icon, bool takeOwnership = false);
    HICON GetNativeIcon() const { return m_nativeIcon; }

    Rect GetMinimizeButtonRect() const;
    Rect GetMaximizeButtonRect() const;
    Rect GetCloseButtonRect() const;

private:
    bool IsMenuBarHit(float x, float y) const;
    bool IsCaptionButtonHit(float x, float y) const;
    int HitTestHoverRegion(float x, float y) const;
    bool EnsureNativeIconBitmap(GraphicsContext& ctx);
    void ApplyHoverRegion(int region, bool forceDirty);
    void SyncCaptionHoverTargets();

    MenuBar m_menuBar;
    std::string m_title;
    std::string m_iconText;
    HICON m_nativeIcon = nullptr;
    bool m_ownsNativeIcon = false;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_nativeIconBitmap;
    bool m_menuChromeDirty = false;
    int m_hoverRegion = -1;
    AnimatedScalar m_minHoverAnim{ 0.0f };
    AnimatedScalar m_maxHoverAnim{ 0.0f };
    AnimatedScalar m_closeHoverAnim{ 0.0f };
};

} // namespace CUI
