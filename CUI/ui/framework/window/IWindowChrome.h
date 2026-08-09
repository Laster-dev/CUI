#pragma once

namespace CUI {

class UIElement;

class IWindowChrome {
public:
    virtual ~IWindowChrome() = default;

    virtual UIElement* GetChromeElement() = 0;
    virtual const UIElement* GetChromeElement() const = 0;
    virtual bool IsInteractiveHit(float x, float y) const = 0;
    virtual bool IsCaptionDragHit(float x, float y, UIElement* treeHit) const = 0;
    virtual bool ConsumeChromeDirty() = 0;
};

} // namespace CUI
