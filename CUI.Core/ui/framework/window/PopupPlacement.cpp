#include "PopupPlacement.h"
#include "PopupHost.h"
#include <algorithm>

namespace CUI {

namespace {
float ClampAxis(float value, float minValue, float maxValue) {
    if (maxValue < minValue) {
        return minValue;
    }
    return std::clamp(value, minValue, maxValue);
}
} // namespace

Rect GetPopupViewportOrDefault(const Rect& fallback) {
    if (PopupHost* host = PopupHost::Current()) {
        const Rect& viewport = host->GetViewport();
        if (!viewport.IsEmpty()) {
            return viewport;
        }
    }
    return fallback;
}

Rect PlacePopupNearAnchor(
    const Rect& anchor,
    float popupWidth,
    float popupHeight,
    const Rect& viewport,
    float gap,
    float margin,
    PopupVerticalPlacement vertical) {
    popupWidth = (std::max)(0.0f, popupWidth);
    popupHeight = (std::max)(0.0f, popupHeight);

    const float viewportLeft = viewport.x + margin;
    const float viewportTop = viewport.y + margin;
    const float viewportRight = viewport.x + viewport.width - margin;
    const float viewportBottom = viewport.y + viewport.height - margin;
    const float availableW = (std::max)(0.0f, viewportRight - viewportLeft);
    const float availableH = (std::max)(0.0f, viewportBottom - viewportTop);
    popupWidth = (std::min)(popupWidth, availableW);
    popupHeight = (std::min)(popupHeight, availableH);
    const float maxX = (std::max)(viewportLeft, viewportRight - popupWidth);
    const float maxY = (std::max)(viewportTop, viewportBottom - popupHeight);

    float x = anchor.x;
    if (x + popupWidth > viewportRight) {
        x = anchor.x + anchor.width - popupWidth;
    }
    x = ClampAxis(x, viewportLeft, maxX);

    float belowY = anchor.y + anchor.height + gap;
    float aboveY = anchor.y - gap - popupHeight;
    float y = belowY;

    const bool belowFits = belowY + popupHeight <= viewportBottom + 0.5f;
    const bool aboveFits = aboveY >= viewportTop - 0.5f;

    switch (vertical) {
    case PopupVerticalPlacement::Above:
        y = aboveY;
        break;
    case PopupVerticalPlacement::Below:
        y = belowY;
        break;
    case PopupVerticalPlacement::AutoFlip:
    default:
        // Rule: if "below" doesn't fit, prefer "above" (even if above also overflows);
        // final y is still clamped to the viewport.
        if (!belowFits) {
            y = aboveY;
        }
        break;
    }

    y = ClampAxis(y, viewportTop, maxY);
    return Rect(x, y, popupWidth, popupHeight);
}

Rect PlacePopupAtPoint(
    Point anchor,
    float popupWidth,
    float popupHeight,
    const Rect& viewport,
    float margin) {
    Rect pseudoAnchor(anchor.x, anchor.y, 0.0f, 0.0f);
    return PlacePopupNearAnchor(
        pseudoAnchor,
        popupWidth,
        popupHeight,
        viewport,
        0.0f,
        margin,
        PopupVerticalPlacement::AutoFlip);
}

} // namespace CUI
