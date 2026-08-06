#pragma once

#include <chrono>
#include <vector>

namespace CUI {

class UIElement;

class AnimationManager {
public:
    using clock = std::chrono::steady_clock;

    static AnimationManager* Current();
    static void SetCurrent(AnimationManager* manager);

    void BeginFrame(clock::time_point now, bool animationActiveBeforeTick);

    float GetDeltaSeconds() const { return m_deltaSeconds; }
    float GetTargetFrameSeconds() const { return m_targetFrameSeconds; }

    void SetTargetFrameSeconds(float seconds);

    void RequestFrame() { m_frameRequested = true; }
    bool HasFrameRequest() const { return m_frameRequested; }
    bool ConsumeFrameRequest();

    void RegisterAnimating(UIElement* element);
    void UnregisterAnimating(UIElement* element);
    bool HasAnimating() const { return !m_animating.empty(); }

    // Ticks registered elements only. Unregisters any that return idle (false).
    bool Tick();

private:
    static AnimationManager* s_current;

    clock::time_point m_lastFrameTime{};
    float m_deltaSeconds = 1.0f / 60.0f;
    float m_targetFrameSeconds = 1.0f / 60.0f;
    bool m_hasLastFrameTime = false;
    bool m_frameRequested = false;
    std::vector<UIElement*> m_animating;
};

} // namespace CUI
