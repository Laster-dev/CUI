#pragma once
#include "../core/Value.h"

namespace CUI {

enum class PopupVerticalPlacement {
    Below,
    Above,
    AutoFlip
};

// Place a popup relative to an anchor rect inside viewport (all in layout/DIP coords).
Rect PlacePopupNearAnchor(
    const Rect& anchor,
    float popupWidth,
    float popupHeight,
    const Rect& viewport,
    float gap = 4.0f,
    float margin = 4.0f,
    PopupVerticalPlacement vertical = PopupVerticalPlacement::AutoFlip);

// Place a popup with its top-left near a point (context menus).
Rect PlacePopupAtPoint(
    Point anchor,
    float popupWidth,
    float popupHeight,
    const Rect& viewport,
    float margin = 4.0f);

Rect GetPopupViewportOrDefault(const Rect& fallback = Rect(0.0f, 0.0f, 10000.0f, 10000.0f));

} // namespace CUI
