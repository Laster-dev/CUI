#include "AnimationManager.h"
#include "FrameScheduler.h"
#include "../controls/UIElement.h"
#include "../render/GraphicsContext.h"
#include <algorithm>
#include <cmath>

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
    m_deltaSeconds = std::clamp(m_deltaSeconds, 1.0f / 240.0f, 0.05f);
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

void AnimationManager::RequestWake(UIElement* element, clock::time_point when) {
    if (!element) {
        return;
    }
    for (auto& wake : m_wakes) {
        if (wake.element == element) {
            wake.when = when;
            if (FrameScheduler* sched = FrameScheduler::Current()) {
                sched->ScheduleFrameAt(when);
            }
            return;
        }
    }
    m_wakes.push_back(WakeEntry{ element, when });
    if (FrameScheduler* sched = FrameScheduler::Current()) {
        sched->ScheduleFrameAt(when);
    }
}

void AnimationManager::CancelWake(UIElement* element) {
    if (!element || m_wakes.empty()) {
        return;
    }
    m_wakes.erase(
        std::remove_if(m_wakes.begin(), m_wakes.end(),
            [element](const WakeEntry& w) { return w.element == element; }),
        m_wakes.end());
}

void AnimationManager::DispatchDueWakes(clock::time_point now) {
    if (m_wakes.empty()) {
        return;
    }
    std::vector<UIElement*> due;
    m_wakes.erase(
        std::remove_if(m_wakes.begin(), m_wakes.end(),
            [&](const WakeEntry& w) {
                if (!w.element || !IsInLiveTree(w.element)) {
                    return true;
                }
                if (w.when <= now) {
                    due.push_back(w.element);
                    return true;
                }
                return false;
            }),
        m_wakes.end());
    for (UIElement* el : due) {
        if (el) {
            el->RequestAnimationTicks();
        }
    }
}

int AnimationManager::GetMsUntilNextWake(clock::time_point now) const {
    if (m_wakes.empty()) {
        return -1;
    }
    auto earliest = m_wakes.front().when;
    for (const auto& wake : m_wakes) {
        if (wake.when < earliest) {
            earliest = wake.when;
        }
    }
    if (earliest <= now) {
        return 0;
    }
    const double ms = std::chrono::duration<double, std::milli>(earliest - now).count();
    return static_cast<int>(std::ceil(ms));
}

void AnimationManager::RegisterAnimating(UIElement* element) {
    if (!element) {
        return;
    }
    if (m_liveRoot && !IsInLiveTree(element)) {
        return;
    }
    if (std::find(m_animating.begin(), m_animating.end(), element) != m_animating.end()) {
        return;
    }
    m_animating.push_back(element);
    m_frameRequested = true;
    if (FrameScheduler* sched = FrameScheduler::Current()) {
        sched->ScheduleFrame();
    }
}

void AnimationManager::UnregisterAnimating(UIElement* element) {
    if (!element || m_animating.empty()) {
        return;
    }
    m_animating.erase(std::remove(m_animating.begin(), m_animating.end(), element), m_animating.end());
}

bool AnimationManager::IsInLiveTree(const UIElement* element) const {
    if (!element) {
        return false;
    }
    if (!m_liveRoot) {
        return true;
    }
    for (const UIElement* walk = element; walk; walk = walk->GetParent()) {
        if (walk == m_liveRoot) {
            return true;
        }
    }
    return false;
}

bool AnimationManager::IsRegistered(const UIElement* element) const {
    if (!element || m_animating.empty()) {
        return false;
    }
    return std::find(m_animating.begin(), m_animating.end(), element) != m_animating.end();
}

void AnimationManager::SetLiveRoot(UIElement* root) {
    m_liveRoot = root;
    PruneDetachedAnimators();
}

void AnimationManager::PruneDetachedAnimators() {
    if (!m_liveRoot) {
        return;
    }
    for (UIElement* el : std::vector<UIElement*>(m_animating)) {
        if (el && !IsInLiveTree(el)) {
            el->CancelAnimationTicks();
        }
    }
    m_wakes.erase(
        std::remove_if(m_wakes.begin(), m_wakes.end(),
            [this](const WakeEntry& w) {
                return !w.element || !IsInLiveTree(w.element);
            }),
        m_wakes.end());
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
        if (!IsInLiveTree(el)) {
            el->CancelAnimationTicks();
            CancelWake(el);
            continue;
        }
        if (el->OnAnimationTick()) {
            any = true;
        } else {
            el->CancelAnimationTicks();
        }
    }
    // Do NOT keep the pump alive solely because the list is non-empty: a re-register
    // during Tick used to leave orphans and spin CommitFrame forever (整帧 storm).
    return any;
}

void AnimationManager::CollectAnimatingBounds(Rect& dirtyRect, bool& hasDirty) const {
    for (UIElement* el : m_animating) {
        if (!el || !IsInLiveTree(el)) {
            continue;
        }
        el->CollectSelfAnimationBounds(dirtyRect, hasDirty);
    }
}

void AnimationManager::CollectComposePresentBounds(Rect& dirtyRect, bool& hasDirty) const {
    for (UIElement* el : m_animating) {
        if (!el || !IsInLiveTree(el) || !el->IsComposeOnlyAnimation()) {
            continue;
        }
        const Rect bounds = el->GetBounds();
        if (bounds.IsEmpty()) {
            continue;
        }
        dirtyRect = hasDirty ? dirtyRect.Union(bounds) : bounds;
        hasDirty = true;
    }
}

bool AnimationManager::HasComposeOnlyAnimating() const {
    for (UIElement* el : m_animating) {
        if (el && IsInLiveTree(el) && el->IsComposeOnlyAnimation()) {
            return true;
        }
    }
    return false;
}

bool AnimationManager::HasSceneContributingAnimators() const {
    for (UIElement* el : m_animating) {
        if (!el || !IsInLiveTree(el)) {
            continue;
        }
        if (!el->IsComposeOnlyAnimation()) {
            return true;
        }
    }
    return false;
}

bool AnimationManager::FlushComposePresent(GraphicsContext& ctx) {
    bool any = false;
    for (UIElement* el : m_animating) {
        if (!el || !IsInLiveTree(el) || !el->IsComposeOnlyAnimation()) {
            continue;
        }
        if (el->ComposePresent(ctx)) {
            any = true;
        }
    }
    // Do not Commit/Present here — Window::CommitFrame / EndDraw owns publishing.
    return any;
}

} // namespace CUI
