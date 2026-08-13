#pragma once

#include <algorithm>
#include <cmath>

namespace CUI {

struct AnimationSpec {
    float responseAt60Hz = 0.20f;
    float epsilon = 0.01f;
    // 0 = unlimited (classic exponential settle). >0 snaps to target after this
    // many seconds — only opt in when a hard CSS-like duration is desired.
    float maxDurationSeconds = 0.0f;
};

// WinUI 3 flyout/menu enter: cubic ease-out over ~220ms (not exponential jump).
struct PopupReveal {
    static constexpr AnimationSpec kSpec{ 0.20f, 0.005f, 0.22f };
};

class AnimationSystem {
public:
    static float BlendFactor(float responseAt60Hz, float dtSeconds);

    static bool Step(float& current, float target, float dtSeconds, const AnimationSpec& spec) {
        return Step(current, target, dtSeconds, spec.responseAt60Hz, spec.epsilon);
    }

    static bool Step(float& current, float target, float dtSeconds, float responseAt60Hz, float epsilon);

    // Finite-duration ease-out (CSS-like). Returns false when finished.
    static bool StepDuration(float& current, float from, float target, float elapsed, float durationSeconds);
};

class AnimatedScalar {
public:
    explicit AnimatedScalar(float value = 0.0f) : m_current(value), m_target(value) {}

    float Current() const { return m_current; }
    float Target() const { return m_target; }

    void Reset(float value) {
        m_current = value;
        m_target = value;
        m_elapsed = 0.0f;
        m_from = value;
    }

    void SetTarget(float value) {
        if (std::abs(value - m_target) > 0.0001f) {
            m_from = m_current;
            m_elapsed = 0.0f;
        }
        m_target = value;
    }

    bool Tick(float dtSeconds, const AnimationSpec& spec) {
        if (std::abs(m_target - m_current) <= spec.epsilon) {
            const bool changed = std::abs(m_target - m_current) > 0.0001f;
            m_current = m_target;
            m_elapsed = 0.0f;
            return changed;
        }

        // Fixed-duration path: total play time is always maxDurationSeconds,
        // independent of how far we travel or how many UI items are involved.
        if (spec.maxDurationSeconds > 0.0f) {
            const float prev = m_current;
            m_elapsed += std::clamp(dtSeconds, 0.0f, 0.05f);
            const bool moving = AnimationSystem::StepDuration(
                m_current, m_from, m_target, m_elapsed, spec.maxDurationSeconds);
            if (!moving) {
                m_current = m_target;
                m_elapsed = 0.0f;
            }
            return moving || std::abs(m_current - prev) > 0.0001f;
        }

        const bool changed = AnimationSystem::Step(m_current, m_target, dtSeconds, spec);
        if (!changed) {
            m_elapsed = 0.0f;
            return false;
        }

        m_elapsed += std::clamp(dtSeconds, 0.0f, 0.05f);
        return true;
    }

    bool IsAnimating(float epsilon = 0.01f) const {
        return std::abs(m_target - m_current) > epsilon;
    }

private:
    float m_current = 0.0f;
    float m_target = 0.0f;
    float m_from = 0.0f;
    float m_elapsed = 0.0f;
};

} // namespace CUI
