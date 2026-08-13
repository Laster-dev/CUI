#pragma once
#include "../core/Value.h"
#include "../render/GraphicsContext.h"
#include <vector>
#include <windows.h>

namespace CUI {

class UIElement;

// Window-owned popup surface. Menus / dropdowns / flyouts / pickers register here
// so hit-test, light-dismiss, overlay paint, and flat (opaque) chrome stay consistent.
class IPopup {
public:
    virtual ~IPopup() = default;

    virtual bool IsPopupOpen() const = 0;
    virtual Rect GetPopupBounds() const = 0;
    // Click on popup or its anchor should not light-dismiss this popup.
    virtual bool HitDismissExempt(float x, float y) const = 0;
    virtual UIElement* HitTestPopup(float x, float y) = 0;
    virtual void RenderPopup(GraphicsContext& ctx) = 0;
    virtual void OnLightDismiss() = 0;
    virtual bool TickPopupAnimation() { return false; }
    // Overlay-composed children painted in RenderPopup. Listed so PopupHost can
    // re-arm animation ticks if a RequestAnimationTicks raced attach.
    virtual void CollectPopupOwnedElements(std::vector<UIElement*>& out) const { (void)out; }
    // True when the popup paints on its own top-level HWND (outside owner client).
    virtual bool IsExternallyHosted() const { return false; }
    virtual void CollectPopupDirty(Rect& dirtyRect, bool& hasDirty) const {
        if (!IsPopupOpen()) return;
        Rect b = GetPopupBounds().Inflate(4.0f);
        if (b.IsEmpty()) return;
        dirtyRect = hasDirty ? dirtyRect.Union(b) : b;
        hasDirty = true;
    }
};

class PopupHost {
public:
    static PopupHost* Current();
    static void SetCurrent(PopupHost* host);

    void Open(IPopup* popup);
    void Close(IPopup* popup);
    void CloseAll();
    void CloseAllExcept(IPopup* exempt);

    // Returns true if any popup was dismissed.
    bool DismissIfOutside(float x, float y);

    UIElement* HitTest(float x, float y);
    void Render(GraphicsContext& ctx);
    bool TickAnimations();
    void CollectDirty(Rect& dirtyRect, bool& hasDirty) const;

    void SetViewport(const Rect& viewport) { m_viewport = viewport; }
    const Rect& GetViewport() const { return m_viewport; }

    void SetOwnerHwnd(HWND hwnd) { m_ownerHwnd = hwnd; }
    HWND GetOwnerHwnd() const { return m_ownerHwnd; }

    bool HasOpenPopups() const { return !m_open.empty(); }
    IPopup* GetTop() const { return m_open.empty() ? nullptr : m_open.back(); }
    // Light-dismiss the top-most popup (Esc). Returns true if one was closed.
    bool DismissTop();

private:
    std::vector<IPopup*> m_open;
    Rect m_viewport;
    HWND m_ownerHwnd = nullptr;
    static PopupHost* s_current;
};

} // namespace CUI
