#pragma once
#include "../core/Value.h"
#include <vector>
#include <memory>

namespace CUI {

class UIElement;

class LayoutEngine {
public:
    static Size MeasureElement(UIElement* element, Size availableSize);
    static void ArrangeElement(UIElement* element, Rect finalRect);

    static Size MeasureFlexPanel(UIElement* panel, Size availableSize, Orientation orientation, Alignment align, float gap);
    static void ArrangeFlexPanel(UIElement* panel, Rect finalRect, Orientation orientation, Alignment align, float gap);
};

} // namespace CUI
