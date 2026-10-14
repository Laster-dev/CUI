#pragma once

#include "InputTypes.h"

namespace CUI {

class UIElement;

enum class RoutedEventPhase {
    Tunnel,
    Target,
    Bubble
};

enum class RoutedEventType {
    PointerPressed,
    PointerReleased,
    PointerMoved,
    KeyDown,
    KeyUp
};

struct RoutedEventArgs {
    RoutedEventType type = RoutedEventType::PointerPressed;
    RoutedEventPhase phase = RoutedEventPhase::Tunnel;
    Point position{};
    int keyCode = 0;
    Key key = Key::None;
    bool handled = false;
    UIElement* originalSource = nullptr;
};

} // namespace CUI
