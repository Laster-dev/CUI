#pragma once

#include "Control.h"
#include "MenuBar.h"
#include "../window/IWindowChrome.h"

namespace CUI {

class WindowTitleBar : public Control, public IWindowChrome {
public:
    WindowTitleBar();
    virtual ~WindowTitleBar() = default;

    virtual const char* GetClassName() const override { return "WindowTitleBar"; }
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnBlur() override;
    virtual UIElement* HitTest(float x, float y) override;

    virtual UIElement* GetChromeElement() override { return this; }
    virtual const UIElement* GetChromeElement() const override { return this; }
    virtual bool IsInteractiveHit(float x, float y) const override;
    virtual bool IsCaptionDragHit(float x, float y, UIElement* treeHit) const override;
    virtual LRESULT HitTestNonClient(float x, float y) const override;
    virtual bool ConsumeChromeDirty() override;

    MenuBar& GetMenuBar() { return m_menuBar; }
    const MenuBar& GetMenuBar() const { return m_menuBar; }

    void SetTitle(const std::string& title);
    const std::string& GetTitle() const { return m_title; }

    void SetIconText(const std::string& iconText);
    const std::string& GetIconText() const { return m_iconText; }

    Rect GetMinimizeButtonRect() const;
    Rect GetMaximizeButtonRect() const;
    Rect GetCloseButtonRect() const;

private:
    bool IsMenuBarHit(float x, float y) const;
    bool IsCaptionButtonHit(float x, float y) const;
    int HitTestHoverRegion(float x, float y) const;

    MenuBar m_menuBar;
    std::string m_title;
    std::string m_iconText;
    bool m_menuChromeDirty = false;
    int m_hoverRegion = -1;
};

} // namespace CUI
