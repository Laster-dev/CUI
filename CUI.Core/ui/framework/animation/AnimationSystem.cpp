#include "AnimationSystem.h"
#include "../controls/UIElement.h"

namespace CUI {

float AnimationSystem::BlendFactor(float responseAt60Hz, float dtSeconds) {
    responseAt60Hz = std::clamp(responseAt60Hz, 0.0f, 0.999f);
    dtSeconds = std::clamp(dtSeconds, 1.0f / 240.0f, 0.050f);
    // Equivalent to 1-(1-r)^(60*dt); one exp instead of variable-exponent pow.
    const float oneMinus = 1.0f - responseAt60Hz;
    const float k = -60.0f * std::log((std::max)(oneMinus, 1.0e-4f));
    return 1.0f - std::exp(-k * dtSeconds);
}

bool AnimationSystem::Step(float& current, float target, float dtSeconds, float responseAt60Hz, float epsilon) {
    if (!UIElement::AreAnimationsEnabled()) {
        const bool changed = std::abs(target - current) > 0.0001f;
        current = target;
        return changed;
    }

    const float previous = current;
    float delta = target - current;
    if (std::abs(delta) <= epsilon) {
        current = target;
        return std::abs(current - previous) > 0.0001f;
    }

    current += delta * BlendFactor(responseAt60Hz, dtSeconds);
    if (std::abs(target - current) <= epsilon * 1.5f) {
        current = target;
    }
    return std::abs(current - previous) > 0.0001f;
}

bool AnimationSystem::StepDuration(float& current, float from, float target, float elapsed, float durationSeconds) {
    if (!UIElement::AreAnimationsEnabled() || durationSeconds <= 0.0001f) {
        current = target;
        return false;
    }
    const float t = std::clamp(elapsed / durationSeconds, 0.0f, 1.0f);
    // Ease-out cubic — same family as CSS cubic-bezier ease-out.
    const float inv = 1.0f - t;
    const float e = 1.0f - inv * inv * inv;
    current = from + (target - from) * e;
    if (t >= 1.0f) {
        current = target;
        return false;
    }
    return true;
}

} // namespace CUI
