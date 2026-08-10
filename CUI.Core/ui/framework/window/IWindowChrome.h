#pragma once
#include <windows.h>

namespace CUI {

class UIElement;

class IWindowChrome {
public:
    virtual ~IWindowChrome() = default;

    virtual UIElement* GetChromeElement() = 0;
    virtual const UIElement* GetChromeElement() const = 0;
    virtual bool IsInteractiveHit(float x, float y) const = 0;
    virtual bool IsCaptionDragHit(float x, float y, UIElement* treeHit) const = 0;
    virtual LRESULT HitTestNonClient(float x, float y) const = 0;
    virtual bool ConsumeChromeDirty() = 0;

    // Caption buttons return HTMIN/MAX/CLOSE, so hover arrives as NC mouse
    // messages — not client WM_MOUSEMOVE. Chrome must update hover here.
    virtual void NotifyNonClientMouseMove(float x, float y) {}
    virtual void NotifyNonClientMouseLeave() {}
};

} // namespace CUI
