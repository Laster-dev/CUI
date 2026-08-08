#pragma once
#include "UIElement.h"
#include "../animation/AnimationManager.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace CUI {

// Overlay-style scrollbar visibility: keep layout/hit-test, hide chrome after idle.
// Show/hide are snapped inside Tick (not multi-frame faded): fading re-entered the
// animation pump and re-painted tall track strips after idle — matching
// "卡 when hidden / smooth while the bar is still visible".
class ScrollbarAutoHide {
public:
    using clock = std::chrono::steady_clock;
    static constexpr float kIdleHideSeconds = 1.5f;

    void NotifyActivity(UIElement* wakeOwner = nullptr) {
        m_idleSeconds = 0.0f;
        m_targetOpacity = 1.0f;
        // Low-performance mode: snap immediately (no animation ticks).
        // Otherwise leave opacity for Tick so OnAnimationTick's prev/new dirty fires.
        if (!UIElement::AreAnimationsEnabled()) {
            m_opacity = 1.0f;
        }
        ArmIdleWake(wakeOwner);
    }

    void SetPointerOver(bool over, UIElement* wakeOwner = nullptr) {
        if (m_pointerOver == over) {
            return;
        }
        m_pointerOver = over;
        if (over) {
            NotifyActivity(wakeOwner);
        } else {
            // Resume idle hide countdown after leaving the track.
            ArmIdleWake(wakeOwner);
        }
    }

    void SetDragging(bool dragging, UIElement* wakeOwner = nullptr) {
        if (m_dragging == dragging) {
            return;
        }
        m_dragging = dragging;
        if (dragging) {
            NotifyActivity(wakeOwner);
        } else {
            ArmIdleWake(wakeOwner);
        }
    }

    bool PointerOver() const { return m_pointerOver; }
    bool Dragging() const { return m_dragging; }
    float Opacity() const { return m_opacity; }
    bool IsDrawn() const { return m_opacity > 0.01f; }

    void ArmIdleWake(UIElement* wakeOwner) {
        const bool hold = m_pointerOver || m_dragging;
        if (!wakeOwner || hold || m_targetOpacity <= 0.01f) {
            return;
        }
        m_wakeOwner = wakeOwner;
        m_idleDeadline = clock::now()
            + std::chrono::duration_cast<clock::duration>(
                std::chrono::duration<float>(kIdleHideSeconds));
        m_hasIdleDeadline = true;
        if (AnimationManager* mgr = AnimationManager::Current()) {
            mgr->RequestWake(wakeOwner, m_idleDeadline);
        }
    }

    void ClearIdleWake() {
        m_hasIdleDeadline = false;
        if (m_wakeOwner) {
            if (AnimationManager* mgr = AnimationManager::Current()) {
                mgr->CancelWake(m_wakeOwner);
            }
        }
    }

    // Snap show/hide. Returns true only if opacity still disagrees with target
    // (should be rare; callers keep RequestAnimationTicks while true).
    bool Tick(float dt) {
        (void)dt;
        const bool hold = m_pointerOver || m_dragging;

        if (hold) {
            m_idleSeconds = 0.0f;
            m_targetOpacity = 1.0f;
            m_opacity = 1.0f;
            ClearIdleWake();
            return false;
        }

        if (m_hasIdleDeadline && clock::now() >= m_idleDeadline) {
            m_targetOpacity = 0.0f;
            m_hasIdleDeadline = false;
        }

        // Snap — no exponential fade (that was the idle hitch).
        if (std::abs(m_opacity - m_targetOpacity) > 0.001f) {
            m_opacity = m_targetOpacity;
        }
        return NeedsTicks();
    }

    // True only while opacity disagrees with target — not during idle wait.
    bool NeedsTicks() const {
        return std::abs(m_opacity - m_targetOpacity) > 0.005f;
    }

private:
    float m_opacity = 0.0f;
    float m_targetOpacity = 0.0f;
    float m_idleSeconds = 0.0f;
    bool m_pointerOver = false;
    bool m_dragging = false;
    bool m_hasIdleDeadline = false;
    clock::time_point m_idleDeadline{};
    UIElement* m_wakeOwner = nullptr;
};

} // namespace CUI
