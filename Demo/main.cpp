#ifndef NOMINMAX
#define NOMINMAX
#endif
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#include "framework/core/Widgets.h"
#include "CUI.h"   // 伞形头：一个 include 拿到全部控件、DSL、窗口与渲染能力
#include <iostream>

using namespace CUI;
using namespace CUI::DSL;

int main() {
    CUI::Window window;
    int clickCount = 0;
    CUI::Widgets::Ref titleBar = Widgets::WindowTitleBar().Title("demo").Shared();
    constexpr const char* kSvgStar = R"svg(
<svg t="1787033108092" class="icon" viewBox="0 0 1024 1024" version="1.1" xmlns="http://www.w3.org/2000/svg" p-id="1334">
  <path d="M0 0m0 0l1024 0q0 0 0 0l0 1024q0 0 0 0l-1024 0q0 0 0 0l0-1024q0 0 0 0Z" fill="#E5E5E5" fill-opacity="0" p-id="1335"></path>
  <path d="M359.537778 413.411556c0 13.255111 6.940444 25.543111 18.204444 32.142222l116.053334 68.551111a35.669333 35.669333 0 0 0 36.408888 0l116.053334-68.551111a37.319111 37.319111 0 0 0 18.204444-32.142222v-137.102223a37.319111 37.319111 0 0 0-18.204444-32.142222L530.204444 175.616a35.612444 35.612444 0 0 0-36.408888 0L377.742222 244.167111a37.319111 37.319111 0 0 0-18.204444 32.199111v137.045334z m188.928 334.222222v-137.045334c0-13.255111 6.940444-25.543111 18.204444-32.142222L682.666667 509.895111a35.612444 35.612444 0 0 1 36.352 0l116.053333 68.551111c11.264 6.599111 18.204444 18.887111 18.204444 32.142222v137.102223a37.319111 37.319111 0 0 1-18.204444 32.142222l-116.053333 68.494222a35.669333 35.669333 0 0 1-36.352 0l-116.053334-68.494222a37.319111 37.319111 0 0 1-18.204444-32.199111z m-377.799111 0c0 13.312 6.940444 25.6 18.204444 32.199111l116.053333 68.494222a35.612444 35.612444 0 0 0 36.352 0l116.053334-68.494222a37.319111 37.319111 0 0 0 18.204444-32.199111v-137.045334a37.319111 37.319111 0 0 0-18.204444-32.142222L341.333333 509.895111a35.669333 35.669333 0 0 0-36.352 0l-116.053333 68.551111a37.319111 37.319111 0 0 0-18.204444 32.142222v137.102223z" fill="var(--foreground)" p-id="1336"></path>
</svg>
)svg";

    titleBar.IconText(kSvgStar);
    auto counterLabel = Text("Click count: 0")
        .FontSize(24.0f)
        .AlignHorizontal(Alignment::Center)
        .ForegroundToken(ThemeTokenId::TextSecondary)
        .FontWeight(FontWeight::SemiBold);

    CUI::Widgets::Ref clickButton = Widgets::Button("Click Me!")
        .FontSize(16.0f)
        .Width(160.0f)
        .Height(48.0f)
        .AlignHorizontal(Alignment::Center)
        .OnClick([counterLabel, &clickCount](UIElement*) {
        clickCount++;
        counterLabel.Shared()->SetText("Click count: " + std::to_string(clickCount));
            }).Shared();

    CUI::Widgets::Ref resetButton = Widgets::Button("Reset")
        .FontSize(14.0f)
        .Width(120.0f)
        .Background(Value::ParseColor("#E53935"))
        .HoverBackground(Value::ParseColor("#D32F2F"))
        .PressedBackground(Value::ParseColor("#B71C1C"))
        .Foreground(Color::White)
        .AlignHorizontal(Alignment::Center)
        .OnClick([counterLabel, &clickCount](UIElement*) {
        clickCount = 0;
        counterLabel.Shared()->SetText("Click count: 0");
            }).Shared();
    CUI::Widgets::Ref ThemeModeRange = Widgets::SegmentedControl()
        .Width(120.0f)
        .Margin(2, 2, 10, 2)
        .AlignHorizontal(Alignment::Center)
        .Shared();
    ThemeModeRange->AddItem("Dark");
    ThemeModeRange->AddItem("Light");
    ThemeModeRange->OnSelectionChanged().Connect([](SegmentedControl*, int, const std::string& item) {
        if (auto* window = Window::Current()) {
            window->SetThemeMode(item == "Dark" ? ThemeMode::Dark : ThemeMode::Light);
        }
    });
    // Layout
    auto root = Column(20, {
        titleBar,
        counterLabel,
        clickButton,
        resetButton,
        ThemeModeRange
        })
        .Align(Alignment::Center)
        .Shared();
    window.Fluent()
        .Title("CUI Counter Demo")
        .Size(400, 300)
        .Root(root)
        .Build()
        .Show()
        .Run();
    return 0;
}
