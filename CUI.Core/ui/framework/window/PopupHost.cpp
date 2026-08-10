#include "PopupHost.h"
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
    // One primary popup stack: opening a new root popup closes others unless already open.
    auto it = std::find(m_open.begin(), m_open.end(), popup);
    if (it != m_open.end()) return;

    // Close siblings that are not ancestors/exempt — keep it simple: close all others.
    CloseAllExcept(nullptr);
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
        if (UIElement* hit = p->HitTestPopup(x, y)) {
            return hit;
        }
    }
    return nullptr;
}

void PopupHost::Render(GraphicsContext& ctx) {
    for (IPopup* p : m_open) {
        if (p && p->IsPopupOpen()) {
            p->RenderPopup(ctx);
        }
    }
}

bool PopupHost::TickAnimations() {
    bool any = false;
    for (IPopup* p : m_open) {
        if (p && p->TickPopupAnimation()) {
            any = true;
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
        if (p) p->CollectPopupDirty(dirtyRect, hasDirty);
    }
}

} // namespace CUI
