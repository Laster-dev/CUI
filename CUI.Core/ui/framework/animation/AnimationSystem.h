#pragma once

#include "AnimationService.h"

namespace CUI {

/**
 * @brief 保持向后兼容的静态动画代理。内部所有计算完全委托给 AnimationService 中央服务。
 */
class AnimationSystem {
public:
    static float BlendFactor(float responseAt60Hz, float dtSeconds) {
        return AnimationService::BlendFactor(responseAt60Hz, dtSeconds);
    }

    static bool Step(float& current, float target, float dtSeconds, const AnimationSpec& spec) {
        return AnimationService::Step(current, target, dtSeconds, spec);
    }

    static bool Step(float& current, float target, float dtSeconds, float responseAt60Hz, float epsilon = 0.01f) {
        return AnimationService::Step(current, target, dtSeconds, responseAt60Hz, epsilon);
    }

    static bool StepDuration(float& current, float from, float target, float elapsed, float durationSeconds, EasingType easing = EasingType::EaseOutCubic) {
        return AnimationService::StepDuration(current, from, target, elapsed, durationSeconds, easing);
    }
};

/**
 * @brief 标量浮点平滑动画过渡控制器。统一基于 AnimationService 进行算法步进。
 */
class AnimatedScalar {
public:
    explicit AnimatedScalar(float value = 0.0f) : m_current(value), m_target(value), m_from(value) {}

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

        if (spec.maxDurationSeconds > 0.0f) {
            const float prev = m_current;
            m_elapsed += std::clamp(dtSeconds, 0.0f, 0.05f);
            const bool moving = AnimationService::StepDuration(
                m_current, m_from, m_target, m_elapsed, spec.maxDurationSeconds, spec.easing);
            if (!moving) {
                m_current = m_target;
                m_elapsed = 0.0f;
            }
            return moving || std::abs(m_current - prev) > 0.0001f;
        }

        const bool changed = AnimationService::Step(m_current, m_target, dtSeconds, spec);
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
