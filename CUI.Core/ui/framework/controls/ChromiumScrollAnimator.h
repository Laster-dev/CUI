#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace CUI {

class ChromiumScrollAnimator {
public:
    void Reset(float value) {
        m_current = value;
        m_initial = value;
        m_target = value;
        m_elapsed = 0.0;
        m_totalDuration = 0.0;
        m_lastRetarget = 0.0;
        m_active = false;
        SetEaseInOut();
    }

    void ScrollBy(float delta, float minValue, float maxValue) {
        float newTarget = std::clamp(m_target + delta, minValue, maxValue);
        if (!m_active) {
            m_initial = m_current;
            m_target = newTarget;
            m_lastRetarget = m_elapsed;
            m_totalDuration = m_elapsed + SegmentDuration(m_target - m_initial);
            m_active = std::abs(m_target - m_initial) >= kEpsilon;
            SetEaseInOut();
            return;
        }
        UpdateTarget(newTarget);
    }

    void JumpTo(float value) {
        Reset(value);
    }

    void ClampTo(float minValue, float maxValue) {
        m_current = std::clamp(m_current, minValue, maxValue);
        m_initial = std::clamp(m_initial, minValue, maxValue);
        m_target = std::clamp(m_target, minValue, maxValue);
    }

    bool Tick(double dtSeconds, float minValue, float maxValue) {
        if (!m_active) return false;

        m_elapsed += std::clamp(dtSeconds, 0.0005, 0.050);
        m_current = std::clamp(GetValue(m_elapsed), minValue, maxValue);

        if (m_elapsed >= m_totalDuration || std::abs(m_current - m_target) < kEpsilon) {
            m_current = std::clamp(m_target, minValue, maxValue);
            m_active = false;
        }
        return true;
    }

    float Current() const { return m_current; }
    float Target() const { return m_target; }
    bool IsActive() const { return m_active; }

private:
    struct CubicBezier {
        double x1 = 0.42;
        double y1 = 0.0;
        double x2 = 0.58;
        double y2 = 1.0;

        static double SampleCurve(double a1, double a2, double t) {
            double c = 3.0 * a1;
            double b = 3.0 * (a2 - a1) - c;
            double a = 1.0 - c - b;
            return ((a * t + b) * t + c) * t;
        }

        static double SampleDerivative(double a1, double a2, double t) {
            double c = 3.0 * a1;
            double b = 3.0 * (a2 - a1) - c;
            double a = 1.0 - c - b;
            return (3.0 * a * t + 2.0 * b) * t + c;
        }

        double SolveCurveX(double x) const {
            if (x <= 0.0) return 0.0;
            if (x >= 1.0) return 1.0;

            // Direct 2-iteration Newton-Raphson for O(1) ultra-fast Bezier root finding
            double t = x;
            for (int i = 0; i < 2; ++i) {
                double xAtT = SampleCurve(x1, x2, t) - x;
                double d = SampleDerivative(x1, x2, t);
                if (std::abs(d) < 1e-6) break;
                t -= xAtT / d;
            }
            return std::clamp(t, 0.0, 1.0);
        }

        double GetValue(double x) const {
            if (x <= 0.0) return 0.0;
            if (x >= 1.0) return 1.0;
            return SampleCurve(y1, y2, SolveCurveX(x));
        }

        double Velocity(double x) const {
            if (x <= 0.0) x = 0.0;
            if (x >= 1.0) x = 1.0;
            double t = SolveCurveX(x);
            double dx = SampleDerivative(x1, x2, t);
            if (std::abs(dx) < 1e-7) return 0.0;
            return SampleDerivative(y1, y2, t) / dx;
        }
    };

    static constexpr double kConstantDuration = 9.0;
    static constexpr double kDurationDivisor = 60.0;
    static constexpr double kInverseDeltaRampStartPx = 120.0;
    static constexpr double kInverseDeltaRampEndPx = 480.0;
    static constexpr double kInverseDeltaMinDuration = 6.0;
    static constexpr double kInverseDeltaMaxDuration = 12.0;
    static constexpr double kInverseDeltaSlope =
        (kInverseDeltaMinDuration - kInverseDeltaMaxDuration) /
        (kInverseDeltaRampEndPx - kInverseDeltaRampStartPx);
    static constexpr double kInverseDeltaOffset =
        kInverseDeltaMaxDuration - kInverseDeltaRampStartPx * kInverseDeltaSlope;
    static constexpr double kEpsilon = 0.01;

    void SetEaseInOut() {
        m_bezier = CubicBezier{ 0.42, 0.0, 0.58, 1.0 };
    }

    void SetEaseInOutWithInitialSlope(double slope) {
        slope = std::clamp(slope, -1000.0, 1000.0);
        m_bezier = CubicBezier{ 0.42, 0.42 * slope, 0.58, 1.0 };
    }

    static double SegmentDuration(float delta) {
        double duration = kInverseDeltaOffset + std::abs(delta) * kInverseDeltaSlope;
        duration = std::clamp(duration, kInverseDeltaMinDuration, kInverseDeltaMaxDuration);
        return duration / kDurationDivisor;
    }

    double CalculateVelocity(double t) const {
        double duration = m_totalDuration - m_lastRetarget;
        if (duration <= kEpsilon) return 0.0;
        double progress = (t - m_lastRetarget) / duration;
        double slope = m_bezier.Velocity(progress);
        return slope * ((m_target - m_initial) / duration);
    }

    double VelocityBasedDurationBound(float newDelta, double velocity) const {
        if (std::abs(newDelta) < kEpsilon) return 0.0;
        if (std::abs(velocity) < kEpsilon) return std::numeric_limits<double>::infinity();
        double bound = (newDelta / velocity) * 2.5;
        return bound < 0.0 ? std::numeric_limits<double>::infinity() : bound;
    }

    double BoundedSegmentDuration(float newDelta, double t) const {
        return (std::min)(SegmentDuration(newDelta), VelocityBasedDurationBound(newDelta, CalculateVelocity(t)));
    }

    float GetValue(double t) const {
        double duration = m_totalDuration - m_lastRetarget;
        double localT = t - m_lastRetarget;
        if (duration <= 0.0 || localT >= duration) return m_target;
        if (localT <= 0.0) return m_initial;

        double progress = m_bezier.GetValue(localT / duration);
        return static_cast<float>(m_initial + (m_target - m_initial) * progress);
    }

    void UpdateTarget(float newTarget) {
        double t = (std::max)(m_elapsed, m_lastRetarget);

        if (std::abs(m_target - newTarget) < kEpsilon) {
            m_target = newTarget;
            return;
        }

        float currentPosition = GetValue(t);
        float newDelta = newTarget - currentPosition;
        if (std::abs(newDelta) < kEpsilon) {
            m_current = newTarget;
            m_initial = newTarget;
            m_target = newTarget;
            m_totalDuration = t;
            m_lastRetarget = t;
            m_active = false;
            return;
        }

        double oldDuration = m_totalDuration - m_lastRetarget;
        if (oldDuration <= 0.0) {
            m_initial = currentPosition;
            m_target = newTarget;
            m_totalDuration = t + SegmentDuration(newDelta);
            m_lastRetarget = t;
            SetEaseInOut();
            m_active = true;
            return;
        }

        double newDuration = BoundedSegmentDuration(newDelta, t);
        if (newDuration < kEpsilon) {
            m_current = newTarget;
            m_initial = newTarget;
            m_target = newTarget;
            m_totalDuration = t;
            m_active = false;
            return;
        }

        double velocity = CalculateVelocity(t);
        double newSlope = velocity * (newDuration / newDelta);
        SetEaseInOutWithInitialSlope(newSlope);
        m_current = currentPosition;
        m_initial = currentPosition;
        m_target = newTarget;
        m_totalDuration = t + newDuration;
        m_lastRetarget = t;
        m_active = true;
    }

    CubicBezier m_bezier;
    float m_current = 0.0f;
    float m_initial = 0.0f;
    float m_target = 0.0f;
    double m_elapsed = 0.0;
    double m_totalDuration = 0.0;
    double m_lastRetarget = 0.0;
    bool m_active = false;
};

} // namespace CUI
