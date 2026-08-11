#pragma once
#include "../core/Value.h"
#include "../render/GraphicsContext.h"
#include <algorithm>
#include <cmath>
#include <windows.h>

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

inline HCURSOR CursorForDelta(float dx, float dy, bool canHorizontal, bool canVertical) {
    const bool outV = canVertical && std::abs(dy) > kDeadZoneDip;
    const bool outH = canHorizontal && std::abs(dx) > kDeadZoneDip;
    if (outV && outH) {
        return LoadCursor(nullptr, IDC_SIZEALL);
    }
    if (outV) {
        return LoadCursor(nullptr, IDC_SIZENS);
    }
    if (outH) {
        return LoadCursor(nullptr, IDC_SIZEWE);
    }
    return LoadCursor(nullptr, IDC_SIZEALL);
}

inline void PaintOriginIndicator(
    GraphicsContext& ctx,
    Point origin,
    bool canHorizontal,
    bool canVertical,
    D2D1_COLOR_F fill,
    D2D1_COLOR_F stroke,
    D2D1_COLOR_F arrow) {
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
