#include "AnimationManager.h"
#include <algorithm>

namespace CUI {

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

} // namespace CUI
