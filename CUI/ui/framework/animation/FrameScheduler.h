#pragma once

#include <chrono>
#include <optional>

namespace CUI {

// Independent frame clock (browser rAF / Flutter Choreographer analogue).
// Does not care about mouse messages — only deadlines.
class FrameScheduler {
public:
    using clock = std::chrono::steady_clock;

    static FrameScheduler* Current();
    static void SetCurrent(FrameScheduler* scheduler);

    // Request the next frame slot (paced by DXGI Present when painting).
    void ScheduleFrame();
    // Low-frequency work (caret blink, toast timeout).
    void ScheduleFrameAt(clock::time_point when);
    void CancelScheduled();

    bool HasPending() const;
    // Milliseconds until the next due frame, or -1 if none.
    int GetMsUntilDeadline(clock::time_point now) const;

    // True if a frame should run now. Clears "immediate" request; keeps future ScheduleFrameAt.
    bool ConsumeDue(clock::time_point now);

    void SetMinFrameInterval(clock::duration interval);
    clock::duration GetMinFrameInterval() const { return m_minInterval; }

private:
    static FrameScheduler* s_current;

    bool m_immediate = false;
    std::optional<clock::time_point> m_deadline;
    clock::time_point m_lastConsumed{};
    bool m_hasLastConsumed = false;
    clock::duration m_minInterval{ std::chrono::milliseconds(16) };
};

} // namespace CUI
