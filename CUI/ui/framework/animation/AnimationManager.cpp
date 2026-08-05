#include "AnimationManager.h"
#include "../controls/UIElement.h"
#include <algorithm>

namespace CUI {

AnimationManager* AnimationManager::s_current = nullptr;

AnimationManager* AnimationManager::Current() {
    return s_current;
}

void AnimationManager::SetCurrent(AnimationManager* manager) {
    s_current = manager;
}

void AnimationManager::BeginFrame(clock::time_point now, bool animationActiveBeforeTick) {
    if (!m_hasLastFrameTime || !animationActiveBeforeTick) {
        m_deltaSeconds = m_targetFrameSeconds;
        m_lastFrameTime = now;
        m_hasLastFrameTime = true;
        return;
    }

    m_deltaSeconds = std::chrono::duration<float>(now - m_lastFrameTime).count();
    m_deltaSeconds = std::clamp(m_deltaSeconds, 1.0f / 240.0f, 0.050f);
    m_lastFrameTime = now;
}

void AnimationManager::SetTargetFrameSeconds(float seconds) {
    m_targetFrameSeconds = std::clamp(seconds, 1.0f / 240.0f, 1.0f / 15.0f);
}

bool AnimationManager::ConsumeFrameRequest() {
    bool requested = m_frameRequested;
    m_frameRequested = false;
    return requested;
}

void AnimationManager::RegisterAnimating(UIElement* element) {
    if (!element) {
        return;
    }
    if (std::find(m_animating.begin(), m_animating.end(), element) != m_animating.end()) {
        return;
    }
    m_animating.push_back(element);
    m_frameRequested = true;
}

void AnimationManager::UnregisterAnimating(UIElement* element) {
    if (!element || m_animating.empty()) {
        return;
    }
    m_animating.erase(std::remove(m_animating.begin(), m_animating.end(), element), m_animating.end());
}

bool AnimationManager::Tick() {
    if (m_animating.empty()) {
        return false;
    }

    bool any = false;
    // Snapshot so Request/Cancel during ticks cannot invalidate iteration.
    std::vector<UIElement*> snapshot = m_animating;
    for (UIElement* el : snapshot) {
        if (!el) {
            continue;
        }
        if (std::find(m_animating.begin(), m_animating.end(), el) == m_animating.end()) {
            continue;
        }
        if (el->OnAnimationTick()) {
            any = true;
        } else {
            el->CancelAnimationTicks();
        }
    }
    return any || !m_animating.empty();
}

} // namespace CUI
