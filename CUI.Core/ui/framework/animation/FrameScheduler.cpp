#include "FrameScheduler.h"
#include <algorithm>
#include <cmath>

namespace CUI {

FrameScheduler* FrameScheduler::s_current = nullptr;

FrameScheduler* FrameScheduler::Current() {
    return s_current;
}

void FrameScheduler::SetCurrent(FrameScheduler* scheduler) {
    s_current = scheduler;
}

void FrameScheduler::ScheduleFrame() {
    m_immediate = true;
}

void FrameScheduler::ScheduleFrameAt(clock::time_point when) {
    if (!m_deadline.has_value() || when < *m_deadline) {
        m_deadline = when;
    }
}

void FrameScheduler::CancelScheduled() {
    m_immediate = false;
    m_deadline.reset();
}

bool FrameScheduler::HasPending() const {
    return m_immediate || m_deadline.has_value();
}

int FrameScheduler::GetMsUntilDeadline(clock::time_point now) const {
    // Immediate frames are paced by DXGI Present(1), not by sleeping here.
    if (m_immediate) {
        return 0;
    }
    if (!m_deadline.has_value()) {
        return -1;
    }
    if (*m_deadline <= now) {
        return 0;
    }
    return static_cast<int>(std::ceil(
        std::chrono::duration<double, std::milli>(*m_deadline - now).count()));
}

bool FrameScheduler::ConsumeDue(clock::time_point now) {
    if (m_immediate) {
        m_immediate = false;
        if (m_deadline.has_value() && *m_deadline <= now) {
            m_deadline.reset();
        }
        m_lastConsumed = now;
        m_hasLastConsumed = true;
        return true;
    }

    if (m_deadline.has_value() && *m_deadline <= now) {
        // Low-freq wakes still coalesce against minInterval so caret/toast
        // cannot spin the UI thread if Present is skipped.
        if (m_hasLastConsumed && (now - m_lastConsumed) < m_minInterval) {
            return false;
        }
        m_deadline.reset();
        m_lastConsumed = now;
        m_hasLastConsumed = true;
        return true;
    }
    return false;
}

void FrameScheduler::SetMinFrameInterval(clock::duration interval) {
    m_minInterval = interval;
    if (m_minInterval < std::chrono::milliseconds(1)) {
        m_minInterval = std::chrono::milliseconds(1);
    }
}

} // namespace CUI
