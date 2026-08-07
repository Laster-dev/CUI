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

    // One-shot wake: re-register element when deadline elapses (scrollbar idle hide, etc.).
    void RequestWake(UIElement* element, clock::time_point when);
    void CancelWake(UIElement* element);
    void DispatchDueWakes(clock::time_point now);
    // Milliseconds until the next wake, or -1 if none.
    int GetMsUntilNextWake(clock::time_point now) const;

    void RegisterAnimating(UIElement* element);
    void UnregisterAnimating(UIElement* element);
    bool HasAnimating() const { return !m_animating.empty(); }
    bool HasPendingWake() const { return !m_wakes.empty(); }

    // Only elements under this root stay on the animation pump (detached gallery
    // pages that called RequestAnimationTicks during Build must not spin forever).
    void SetLiveRoot(UIElement* root);
    UIElement* GetLiveRoot() const { return m_liveRoot; }
    bool IsInLiveTree(const UIElement* element) const;
    bool IsRegistered(const UIElement* element) const;
    void PruneDetachedAnimators();

    // Ticks registered elements only. Unregisters any that return idle (false).
    bool Tick();

private:
    struct WakeEntry {
        UIElement* element = nullptr;
        clock::time_point when{};
    };

    static AnimationManager* s_current;

    clock::time_point m_lastFrameTime{};
    float m_deltaSeconds = 1.0f / 60.0f;
    float m_targetFrameSeconds = 1.0f / 60.0f;
    bool m_hasLastFrameTime = false;
    bool m_frameRequested = false;
    UIElement* m_liveRoot = nullptr;
    std::vector<UIElement*> m_animating;
    std::vector<WakeEntry> m_wakes;
};

} // namespace CUI
