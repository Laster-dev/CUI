#include "PopupHost.h"
#include "../controls/UIElement.h"
#include <algorithm>

namespace CUI {

PopupHost* PopupHost::s_current = nullptr;

PopupHost* PopupHost::Current() {
    return s_current;
}

void PopupHost::SetCurrent(PopupHost* host) {
    s_current = host;
}

void PopupHost::Open(IPopup* popup) {
    if (!popup) return;
    // Stack — do not close parents. Nested menus (e.g. BreadcrumbBar "..." inside
    // FilePicker) call Open while the picker is already open; replacing the stack
    // would light-dismiss the picker. Sibling replacement still happens because
    // Window::OnLButtonDown runs DismissIfOutside before the new control Opens.
    auto it = std::find(m_open.begin(), m_open.end(), popup);
    if (it != m_open.end()) return;
    m_open.push_back(popup);
}

void PopupHost::Close(IPopup* popup) {
    if (!popup) return;
    m_open.erase(std::remove(m_open.begin(), m_open.end(), popup), m_open.end());
}

void PopupHost::CloseAll() {
    // Copy — OnLightDismiss may mutate the list.
    std::vector<IPopup*> copy = m_open;
    m_open.clear();
    for (IPopup* p : copy) {
        if (p && p->IsPopupOpen()) {
            p->OnLightDismiss();
        }
    }
}

void PopupHost::CloseAllExcept(IPopup* exempt) {
    std::vector<IPopup*> copy = m_open;
    m_open.clear();
    for (IPopup* p : copy) {
        if (!p) continue;
        if (p == exempt) {
            m_open.push_back(p);
            continue;
        }
        if (p->IsPopupOpen()) {
            p->OnLightDismiss();
        }
    }
}

bool PopupHost::DismissIfOutside(float x, float y) {
    if (m_open.empty()) return false;

    bool dismissed = false;
    std::vector<IPopup*> copy = m_open;
    for (IPopup* p : copy) {
        if (!p || !p->IsPopupOpen()) continue;
        if (p->HitDismissExempt(x, y)) continue;
        p->OnLightDismiss();
        Close(p);
        dismissed = true;
    }
    return dismissed;
}

UIElement* PopupHost::HitTest(float x, float y) {
    // Top-most last opened wins.
    for (auto it = m_open.rbegin(); it != m_open.rend(); ++it) {
        IPopup* p = *it;
        if (!p || !p->IsPopupOpen()) continue;
        if (p->IsExternallyHosted()) continue;
        if (UIElement* hit = p->HitTestPopup(x, y)) {
            return hit;
        }
    }
    return nullptr;
}

void PopupHost::Render(GraphicsContext& ctx) {
    for (IPopup* p : m_open) {
        if (p && p->IsPopupOpen() && !p->IsExternallyHosted()) {
            p->RenderPopup(ctx);
        }
    }
}

bool PopupHost::TickAnimations() {
    bool any = false;
    for (IPopup* p : m_open) {
        if (!p) {
            continue;
        }
        if (p->TickPopupAnimation()) {
            any = true;
        }
        // Safety net: popup-owned controls (TreeView, …) must SetAnimationHost so
        // they can register themselves. If expand/scroll started while the host
        // link was missing, re-arm here so the pump still picks them up.
        std::vector<UIElement*> owned;
        p->CollectPopupOwnedElements(owned);
        for (UIElement* el : owned) {
            if (el && el->HasSelfAnimation()) {
                el->RequestAnimationTicks();
                any = true;
            }
        }
    }
    // Prune closed entries
    m_open.erase(std::remove_if(m_open.begin(), m_open.end(), [](IPopup* p) {
        return !p || !p->IsPopupOpen();
    }), m_open.end());
    return any;
}

void PopupHost::CollectDirty(Rect& dirtyRect, bool& hasDirty) const {
    for (IPopup* p : m_open) {
        if (p && !p->IsExternallyHosted()) {
            p->CollectPopupDirty(dirtyRect, hasDirty);
        }
    }
}

} // namespace CUI
