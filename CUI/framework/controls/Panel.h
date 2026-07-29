#pragma once
#include "UIElement.h"

namespace CUI {

class Panel : public UIElement {
public:
    Panel();
    virtual ~Panel() = default;

    virtual const char* GetClassName() const override { return "Panel"; }
};

class StackPanel : public Panel {
public:
    StackPanel();
    explicit StackPanel(Orientation orientation);
    virtual ~StackPanel() = default;

    virtual const char* GetClassName() const override { return "StackPanel"; }
};

} // namespace CUI
