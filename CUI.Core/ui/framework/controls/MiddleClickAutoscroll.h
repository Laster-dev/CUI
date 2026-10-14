#pragma once
#include "../core/Value.h"
#include "../render/GraphicsContext.h"
#include "../input/InputTypes.h"
#include <algorithm>
#include <cmath>

namespace CUI {
namespace MiddleClickAutoscroll {

// Browser-style middle-click autoscroll (Chrome/Edge):
// origin stays fixed, cursor moves freely, velocity grows with distance.

constexpr float kDeadZoneDip = 15.0f;
constexpr float kIndicatorRadius = 13.0f;

inline float VelocityFromDelta(float deltaDip) {
    const float excess = std::abs(deltaDip) - kDeadZoneDip;
    if (excess <= 0.0f) {
        return 0.0f;
    }
    // Approximate Chromium feel: mild near the dead-zone, faster farther out.
    const float speed = std::pow(excess / 4.0f, 1.85f) * 55.0f;
    return std::copysign(speed, deltaDip);
}

inline CursorType CursorForDelta(float dx, float dy, bool canHorizontal, bool canVertical) {
    const bool outV = canVertical && std::abs(dy) > kDeadZoneDip;
    const bool outH = canHorizontal && std::abs(dx) > kDeadZoneDip;
    if (outV && outH) {
        return CursorType::SizeAll;
    }
    if (outV) {
        return CursorType::SizeNS;
    }
    if (outH) {
        return CursorType::SizeWE;
    }
    return CursorType::SizeAll;
}

inline void PaintOriginIndicator(
    GraphicsContext& ctx,
    Point origin,
    bool canHorizontal,
    bool canVertical,
    Color fill,
    Color stroke,
    Color arrow) {
    const float r = kIndicatorRadius;
    Rect badge(origin.x - r, origin.y - r, r * 2.0f, r * 2.0f);
    ctx.FillRoundedRect(badge, r, fill);
    ctx.DrawRoundedRect(badge, r, stroke, 1.2f);

    const float arm = 5.5f;
    const float tip = 3.0f;
    if (canVertical) {
        // Up / down chevrons
        ctx.DrawLine(Point(origin.x, origin.y - arm - 1.0f), Point(origin.x - tip, origin.y - arm + tip), arrow, 1.4f);
        ctx.DrawLine(Point(origin.x, origin.y - arm - 1.0f), Point(origin.x + tip, origin.y - arm + tip), arrow, 1.4f);
        ctx.DrawLine(Point(origin.x, origin.y + arm + 1.0f), Point(origin.x - tip, origin.y + arm - tip), arrow, 1.4f);
        ctx.DrawLine(Point(origin.x, origin.y + arm + 1.0f), Point(origin.x + tip, origin.y + arm - tip), arrow, 1.4f);
    }
    if (canHorizontal) {
        ctx.DrawLine(Point(origin.x - arm - 1.0f, origin.y), Point(origin.x - arm + tip, origin.y - tip), arrow, 1.4f);
        ctx.DrawLine(Point(origin.x - arm - 1.0f, origin.y), Point(origin.x - arm + tip, origin.y + tip), arrow, 1.4f);
        ctx.DrawLine(Point(origin.x + arm + 1.0f, origin.y), Point(origin.x + arm - tip, origin.y - tip), arrow, 1.4f);
        ctx.DrawLine(Point(origin.x + arm + 1.0f, origin.y), Point(origin.x + arm - tip, origin.y + tip), arrow, 1.4f);
    }
    if (!canHorizontal && !canVertical) {
        ctx.FillRoundedRect(Rect(origin.x - 2.0f, origin.y - 2.0f, 4.0f, 4.0f), 2.0f, arrow);
    }
}

} // namespace MiddleClickAutoscroll
} // namespace CUI
