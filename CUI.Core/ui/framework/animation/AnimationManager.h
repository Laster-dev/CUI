#pragma once

#include "../core/Value.h"
#include <chrono>
#include <vector>

namespace CUI {

class UIElement;
class GraphicsContext;

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
    // Also schedules FrameScheduler::ScheduleFrameAt so the clock does not need mouse input.
    void RequestWake(UIElement* element, clock::time_point when);
    void CancelWake(UIElement* element);
    void DispatchDueWakes(clock::time_point now);
    // Milliseconds until the next wake, or -1 if none.
    int GetMsUntilNextWake(clock::time_point now) const;

    void RegisterAnimating(UIElement* element);
    void UnregisterAnimating(UIElement* element);
    bool HasAnimating() const { return !m_animating.empty(); }
    bool HasPendingWake() const { return !m_wakes.empty(); }

    // Live-tree gate for the animation pump.
    //
    // WHY: NavigationView / gallery pages often call RequestAnimationTicks during
    // Build(). After a page swap those elements are detached (no longer under the
    // window root) but may still be alive via shared_ptr. Without this gate they
    // stay registered, ScheduleFrame never stops, and the app spins at full FPS
    // with nothing on screen (CPU fan / "stuck" feel).
    //
    // RULE: only elements reachable from m_liveRoot may register or stay on the
    // pump. Reachability = layout parent chain, OR AnimationHost chain (see
    // UIElement::SetAnimationHost) for popup/overlay-hosted controls that are
    // painted without being AddChild'd into the layout tree.
    void SetLiveRoot(UIElement* root);
    UIElement* GetLiveRoot() const { return m_liveRoot; }
    // True if element walks to m_liveRoot via GetParent() and/or GetAnimationHost().
    bool IsInLiveTree(const UIElement* element) const;
    bool IsRegistered(const UIElement* element) const;
    void PruneDetachedAnimators();

    // Ticks registered elements only. Unregisters any that return idle (false).
    bool Tick();

    // Dirty rects from the registered animator set only (O(animating), not O(tree)).
    void CollectAnimatingBounds(Rect& dirtyRect, bool& hasDirty) const;
    // Present rects for compose-only animators (no scene cache invalidation).
    void CollectComposePresentBounds(Rect& dirtyRect, bool& hasDirty) const;
    bool HasComposeOnlyAnimating() const;
    // True when some registered animator still contributes scene dirty bounds.
    bool HasSceneContributingAnimators() const;
    // Updates DComp overlays for compose-only animators. Returns true if any ran.
    bool FlushComposePresent(GraphicsContext& ctx);

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
