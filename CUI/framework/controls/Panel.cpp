#include "Panel.h"

namespace CUI {

Panel::Panel() {
    SetProperty("background", Value(D2D1::ColorF(0, 0, 0, 0)));
}

StackPanel::StackPanel() {
    SetProperty("orientation", Value("Vertical"));
    SetProperty("gap", Value(0.0f));
}

StackPanel::StackPanel(Orientation orientation) : StackPanel() {
    SetProperty("orientation", Value(orientation == Orientation::Horizontal ? "Horizontal" : "Vertical"));
}

} // namespace CUI
