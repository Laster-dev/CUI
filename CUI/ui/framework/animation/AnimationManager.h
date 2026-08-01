#pragma once

#include <chrono>

namespace CUI {

class AnimationManager {
public:
    using clock = std::chrono::steady_clock;

    void BeginFrame(clock::time_point now, bool animationActiveBeforeTick);

    float GetDeltaSeconds() const { return m_deltaSeconds; }
    float GetTargetFrameSeconds() const { return m_targetFrameSeconds; }

    void SetTargetFrameSeconds(float seconds);

    void RequestFrame() { m_frameRequested = true; }
    bool ConsumeFrameRequest();

private:
    clock::time_point m_lastFrameTime{};
    float m_deltaSeconds = 1.0f / 60.0f;
    float m_targetFrameSeconds = 1.0f / 60.0f;
    bool m_hasLastFrameTime = false;
    bool m_frameRequested = false;
};

} // namespace CUI
