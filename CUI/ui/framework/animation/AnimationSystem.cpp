#include "AnimationSystem.h"

namespace CUI {

float AnimationSystem::BlendFactor(float responseAt60Hz, float dtSeconds) {
    responseAt60Hz = std::clamp(responseAt60Hz, 0.0f, 0.999f);
    dtSeconds = std::clamp(dtSeconds, 1.0f / 240.0f, 0.050f);
    float frames = dtSeconds * 60.0f;
    return 1.0f - std::pow(1.0f - responseAt60Hz, (std::max)(0.1f, frames));
}

bool AnimationSystem::Step(float& current, float target, float dtSeconds, float responseAt60Hz, float epsilon) {
    float delta = target - current;
    if (std::abs(delta) <= epsilon) {
        current = target;
        return false;
    }

    current += delta * BlendFactor(responseAt60Hz, dtSeconds);
    return true;
}

} // namespace CUI
