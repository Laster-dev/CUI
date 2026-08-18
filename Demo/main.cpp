#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "framework/window/Window.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include <iostream>

using namespace CUI;
using namespace CUI::DSL;

int main() {
    CUI::Window window;
    bool createResult = window.Create("CUI Counter Demo", 400, 300);
    if (!createResult) {
        return 1;
    }
    int clickCount = 0;
    auto titleBar = std::make_shared<WindowTitleBar>();
    titleBar->SetTitle("demo");
    constexpr const char* kSvgStar =
        "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path d=\"M512 64l138.88 281.387 310.4 45.12-224.64 218.987 53.035 309.173L512 772.693 234.325 918.667l53.035-309.173L62.72 390.507l310.4-45.12z\"/>"
        "</svg>";
    titleBar->Icon = kSvgStar;

    auto counterLabel = Text("Click count: 0")
        .FontSize(24.0f)
        .AlignHorizontal(Alignment::Center)
        .FontWeight(FontWeight::SemiBold);

    auto clickButton = Fluent::Button("Click Me!")
        .FontSize(16.0f)
        .Width(160.0f)
        .Height(48.0f)
		.AlignHorizontal(Alignment::Center)
        .OnClick([counterLabel, &clickCount](UIElement*) {
            clickCount++;
            counterLabel->SetText("Click count: " + std::to_string(clickCount));
        });

    auto resetButton = Fluent::Button("Reset")
        .FontSize(14.0f)
        .Width(120.0f)
        .Background("#E53935")
        .Hover("#D32F2F")
        .Pressed("#B71C1C")
        .Foreground(Color::White)
        .AlignHorizontal(Alignment::Center)
        .OnClick([counterLabel, &clickCount](UIElement*) {
            clickCount = 0;
            counterLabel->SetText("Click count: 0");
        });
    // Layout
    auto root = Column(20, {
        titleBar,
        counterLabel,
        clickButton,
        resetButton
    })
    .Align(Alignment::Center);
    window.SetRootElement(root);
    window.Show();
    window.RunMessageLoop();
    return 0;
}
