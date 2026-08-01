#pragma once

#include <algorithm>
#include <cmath>

namespace CUI {

struct AnimationSpec {
    float responseAt60Hz = 0.20f;
    float epsilon = 0.01f;
};

class AnimationSystem {
public:
    static float BlendFactor(float responseAt60Hz, float dtSeconds);

    static bool Step(float& current, float target, float dtSeconds, const AnimationSpec& spec) {
        return Step(current, target, dtSeconds, spec.responseAt60Hz, spec.epsilon);
    }

    static bool Step(float& current, float target, float dtSeconds, float responseAt60Hz, float epsilon);
};

class AnimatedScalar {
public:
    explicit AnimatedScalar(float value = 0.0f) : m_current(value), m_target(value) {}

    float Current() const { return m_current; }
    float Target() const { return m_target; }

    void Reset(float value) {
        m_current = value;
        m_target = value;
    }

    void SetTarget(float value) { m_target = value; }

    bool Tick(float dtSeconds, const AnimationSpec& spec) {
        return AnimationSystem::Step(m_current, m_target, dtSeconds, spec);
    }

    bool IsAnimating(float epsilon = 0.01f) const {
        return std::abs(m_target - m_current) > epsilon;
    }

private:
    float m_current = 0.0f;
    float m_target = 0.0f;
};

} // namespace CUI
