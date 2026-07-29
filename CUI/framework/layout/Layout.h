#pragma once
#include "../core/Value.h"
#include <vector>
#include <memory>
#include <string>

namespace CUI {

class UIElement;

enum class GridUnitType {
    Pixel,
    Auto,
    Star
};

struct GridLength {
    float value = 1.0f;
    GridUnitType unitType = GridUnitType::Star;

    GridLength() = default;
    GridLength(float val, GridUnitType type = GridUnitType::Pixel) : value(val), unitType(type) {}

    static GridLength Auto() { return GridLength(1.0f, GridUnitType::Auto); }
    static GridLength Star(float weight = 1.0f) { return GridLength(weight, GridUnitType::Star); }
    static GridLength Pixel(float px) { return GridLength(px, GridUnitType::Pixel); }
    static GridLength Parse(const std::string& str);
};

struct ColumnDefinition {
    GridLength width = GridLength::Star(1.0f);
    float minWidth = 0.0f;
    float maxWidth = 100000.0f;
    float actualWidth = 0.0f;
    float position = 0.0f;
};

struct RowDefinition {
    GridLength height = GridLength::Star(1.0f);
    float minHeight = 0.0f;
    float maxHeight = 100000.0f;
    float actualHeight = 0.0f;
    float position = 0.0f;
};

enum class Dock {
    Left,
    Top,
    Right,
    Bottom
};

class LayoutEngine {
public:
    static Size MeasureElement(UIElement* element, Size availableSize);
    static void ArrangeElement(UIElement* element, Rect finalRect);

    // Flex / StackPanel
    static Size MeasureFlexPanel(UIElement* panel, Size availableSize, Orientation orientation, Alignment align, float gap);
    static void ArrangeFlexPanel(UIElement* panel, Rect finalRect, Orientation orientation, Alignment align, float gap);

    // Canvas Layout
    static Size MeasureCanvas(UIElement* panel, Size availableSize);
    static void ArrangeCanvas(UIElement* panel, Rect finalRect);

    // Grid Layout
    static Size MeasureGrid(UIElement* panel, Size availableSize);
    static void ArrangeGrid(UIElement* panel, Rect finalRect);

    // WrapPanel Layout
    static Size MeasureWrapPanel(UIElement* panel, Size availableSize);
    static void ArrangeWrapPanel(UIElement* panel, Rect finalRect);

    // DockPanel Layout
    static Size MeasureDockPanel(UIElement* panel, Size availableSize);
    static void ArrangeDockPanel(UIElement* panel, Rect finalRect);

    // UniformGrid Layout
    static Size MeasureUniformGrid(UIElement* panel, Size availableSize);
    static void ArrangeUniformGrid(UIElement* panel, Rect finalRect);
};

} // namespace CUI
