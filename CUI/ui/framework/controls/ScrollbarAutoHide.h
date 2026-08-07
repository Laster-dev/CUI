#pragma once
#include "UIElement.h"
#include "../animation/AnimationManager.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace CUI {

// Overlay-style scrollbar visibility: keep layout/hit-test, fade chrome after idle.
// Show again on scroll activity or pointer-over the track.
class ScrollbarAutoHide {
public:
    using clock = std::chrono::steady_clock;
    static constexpr float kIdleHideSeconds = 1.5f;

    void NotifyActivity(UIElement* wakeOwner = nullptr) {
        m_idleSeconds = 0.0f;
        m_targetOpacity = 1.0f;
        // Low-performance mode disables animation ticks that would fade opacity in.
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
        }
    }

    void SetDragging(bool dragging, UIElement* wakeOwner = nullptr) {
        if (m_dragging == dragging) {
            return;
        }
        m_dragging = dragging;
        if (dragging) {
            NotifyActivity(wakeOwner);
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
        m_idleDeadline = clock::now()
            + std::chrono::duration_cast<clock::duration>(
                std::chrono::duration<float>(kIdleHideSeconds));
        m_hasIdleDeadline = true;
        if (AnimationManager* mgr = AnimationManager::Current()) {
            mgr->RequestWake(wakeOwner, m_idleDeadline);
        }
    }

    // Advance fade / idle countdown. Returns true while ticks are still needed.
    bool Tick(float dt) {
        dt = std::clamp(dt, 0.0f, 0.05f);
        const bool hold = m_pointerOver || m_dragging;

        if (!UIElement::AreAnimationsEnabled()) {
            // Instant show/hide — no exponential fade (animation system is off).
            if (hold) {
                m_idleSeconds = 0.0f;
                m_targetOpacity = 1.0f;
                m_opacity = 1.0f;
                m_hasIdleDeadline = false;
                return false;
            }
            if (m_hasIdleDeadline && clock::now() >= m_idleDeadline) {
                m_targetOpacity = 0.0f;
                m_opacity = 0.0f;
                m_hasIdleDeadline = false;
                return false;
            }
            m_opacity = m_targetOpacity;
            // Waiting for idle deadline: no continuous pump (wake handles it).
            return false;
        }

        if (hold) {
            m_idleSeconds = 0.0f;
            m_targetOpacity = 1.0f;
            m_hasIdleDeadline = false;
        } else if (m_hasIdleDeadline && clock::now() >= m_idleDeadline) {
            m_targetOpacity = 0.0f;
            m_hasIdleDeadline = false;
        } else if (m_targetOpacity > 0.01f && m_hasIdleDeadline) {
            // Keep idleSeconds roughly in sync for diagnostics; wake drives hide.
            m_idleSeconds = kIdleHideSeconds;
        }

        const float prev = m_opacity;
        const float speed = (m_targetOpacity > m_opacity) ? 16.0f : 10.0f;
        const float t = 1.0f - std::exp(-speed * dt);
        m_opacity += (m_targetOpacity - m_opacity) * t;
        if (std::abs(m_opacity - m_targetOpacity) < 0.005f) {
            m_opacity = m_targetOpacity;
        }

        const bool fading = std::abs(m_opacity - prev) > 0.001f
            || std::abs(m_opacity - m_targetOpacity) > 0.005f;
        return fading;
    }

    // True only while opacity is actively changing — not during idle wait.
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
};

} // namespace CUI
