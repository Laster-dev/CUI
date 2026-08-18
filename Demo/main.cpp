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
    constexpr const char* kSvgStar = R"svg(
<svg t="1787033108092" class="icon" viewBox="0 0 1024 1024" version="1.1" xmlns="http://www.w3.org/2000/svg" p-id="1334">
  <path d="M0 0m0 0l1024 0q0 0 0 0l0 1024q0 0 0 0l-1024 0q0 0 0 0l0-1024q0 0 0 0Z" fill="#E5E5E5" fill-opacity="0" p-id="1335"></path>
  <path d="M359.537778 413.411556c0 13.255111 6.940444 25.543111 18.204444 32.142222l116.053334 68.551111a35.669333 35.669333 0 0 0 36.408888 0l116.053334-68.551111a37.319111 37.319111 0 0 0 18.204444-32.142222v-137.102223a37.319111 37.319111 0 0 0-18.204444-32.142222L530.204444 175.616a35.612444 35.612444 0 0 0-36.408888 0L377.742222 244.167111a37.319111 37.319111 0 0 0-18.204444 32.199111v137.045334z m188.928 334.222222v-137.045334c0-13.255111 6.940444-25.543111 18.204444-32.142222L682.666667 509.895111a35.612444 35.612444 0 0 1 36.352 0l116.053333 68.551111c11.264 6.599111 18.204444 18.887111 18.204444 32.142222v137.102223a37.319111 37.319111 0 0 1-18.204444 32.142222l-116.053333 68.494222a35.669333 35.669333 0 0 1-36.352 0l-116.053334-68.494222a37.319111 37.319111 0 0 1-18.204444-32.199111z m-377.799111 0c0 13.312 6.940444 25.6 18.204444 32.199111l116.053333 68.494222a35.612444 35.612444 0 0 0 36.352 0l116.053334-68.494222a37.319111 37.319111 0 0 0 18.204444-32.199111v-137.045334a37.319111 37.319111 0 0 0-18.204444-32.142222L341.333333 509.895111a35.669333 35.669333 0 0 0-36.352 0l-116.053333 68.551111a37.319111 37.319111 0 0 0-18.204444 32.142222v137.102223z" fill="#3A3A3A" p-id="1336"></path>
</svg>
)svg";

    titleBar->Icon = kSvgStar;
    auto counterLabel = Text("Click count: 0")
        .FontSize(24.0f)
        .AlignHorizontal(Alignment::Center)
        .ColorToken(ThemeTokenId::TextSecondary)
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
    auto ThemeModeRange = std::make_shared<SegmentedControl>();
    ThemeModeRange->Width = 120.0f;
    ThemeModeRange->Margin = Thickness(2, 2, 10, 2);
    ThemeModeRange->AddItem("Dark");
    ThemeModeRange->AddItem("Light");
    ThemeModeRange->OnSelectionChanged().Connect([](SegmentedControl*, int, const std::string& item) {
        if (auto* window = Window::Current()) {
            window->SetThemeMode(item == "Dark" ? ThemeMode::Dark : ThemeMode::Light);
        }
        });

    ThemeModeRange->AlignHorizontal = Alignment::Center;
    // Layout
    auto root = Column(20, {
        titleBar,
        counterLabel,
        clickButton,
        resetButton,
        ThemeModeRange
        })
        .Align(Alignment::Center);
    window.SetRootElement(root);
    window.Show();
    window.RunMessageLoop();
    return 0;
}
