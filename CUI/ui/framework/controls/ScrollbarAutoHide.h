#pragma once
#include "UIElement.h"
#include <algorithm>
#include <cmath>

namespace CUI {

// Overlay-style scrollbar visibility: keep layout/hit-test, fade chrome after idle.
// Show again on scroll activity or pointer-over the track.
class ScrollbarAutoHide {
public:
    static constexpr float kIdleHideSeconds = 1.5f;

    void NotifyActivity() {
        m_idleSeconds = 0.0f;
        m_targetOpacity = 1.0f;
        // Low-performance mode disables animation ticks that would fade opacity in.
        if (!UIElement::AreAnimationsEnabled()) {
            m_opacity = 1.0f;
        }
    }

    void SetPointerOver(bool over) {
        if (m_pointerOver == over) {
            return;
        }
        m_pointerOver = over;
        if (over) {
            NotifyActivity();
        }
    }

    void SetDragging(bool dragging) {
        if (m_dragging == dragging) {
            return;
        }
        m_dragging = dragging;
        if (dragging) {
            NotifyActivity();
        }
    }

    bool PointerOver() const { return m_pointerOver; }
    bool Dragging() const { return m_dragging; }
    float Opacity() const { return m_opacity; }
    bool IsDrawn() const { return m_opacity > 0.01f; }

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
                return false;
            }
            if (m_targetOpacity > 0.01f) {
                m_idleSeconds += dt;
                if (m_idleSeconds >= kIdleHideSeconds) {
                    m_targetOpacity = 0.0f;
                    m_opacity = 0.0f;
                    return false;
                }
            }
            m_opacity = m_targetOpacity;
            return m_targetOpacity > 0.01f && m_idleSeconds < kIdleHideSeconds;
        }

        if (hold) {
            m_idleSeconds = 0.0f;
            m_targetOpacity = 1.0f;
        } else if (m_targetOpacity > 0.01f) {
            m_idleSeconds += dt;
            if (m_idleSeconds >= kIdleHideSeconds) {
                m_targetOpacity = 0.0f;
            }
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
        const bool waitingToHide = m_targetOpacity > 0.01f && !hold
            && m_idleSeconds < kIdleHideSeconds;
        return fading || waitingToHide;
    }

    bool NeedsTicks() const {
        if (!UIElement::AreAnimationsEnabled()) {
            const bool hold = m_pointerOver || m_dragging;
            return m_targetOpacity > 0.01f && !hold && m_idleSeconds < kIdleHideSeconds;
        }
        if (std::abs(m_opacity - m_targetOpacity) > 0.005f) {
            return true;
        }
        const bool hold = m_pointerOver || m_dragging;
        return m_targetOpacity > 0.01f && !hold && m_idleSeconds < kIdleHideSeconds;
    }

private:
    float m_opacity = 0.0f;
    float m_targetOpacity = 0.0f;
    float m_idleSeconds = 0.0f;
    bool m_pointerOver = false;
    bool m_dragging = false;
};

} // namespace CUI
