#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<ProgressBar> MakeStretchBar(float height) {
    auto bar = ProgressBarWidget(0.0f, true);
    CUI::DSL::Borrow(bar).Align(Alignment::Stretch);
    CUI::DSL::Borrow(bar).Height(height);
    return bar;
}

} // namespace

Element BuildProgressBarPage() {
    // 确定进度：滑块驱动填充值。
    auto determinate = ProgressBarWidget(40.0f, false);
    CUI::DSL::Borrow(determinate).Align(Alignment::Stretch);
    CUI::DSL::Borrow(determinate).Height(6.0f);

    State<float> value{ 40.0f };
    auto slider = SliderWidget(40.0f, 0.0f, 100.0f);
    CUI::DSL::Borrow(slider).Step(1.0f);
    CUI::DSL::Borrow(slider).Width(280.0f);
    slider->ValueProperty.Bind(value);

    auto bar = determinate;
    value.OnChanged().Connect([bar](const float& v) {
        CUI::DSL::Borrow(bar).Value(v);
    });

    auto statusValue = MakeComputed<std::string>([](float v) {
        return std::format("进度：{:.0f}%", v);
    }, value);
    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto indeterminate = ToggleSwitchTile("不确定模式", false);
    indeterminate->OnToggled().Connect([bar](ToggleSwitch*, bool on) {
        CUI::DSL::Borrow(bar).IsIndeterminate(on);
    });

    SamplePageSpec spec;
    spec.title = "ProgressBar(进度条)";
    spec.subtitle = "指示任务执行进度的水平线条，支持确定值与不确定两种模式。";
    spec.sections = {
        {
            "确定进度",
            "拖动滑块改变进度值；.IsIndeterminate(false) 时按数值绘制填充。",
            Column(12, {
                determinate,
                Row(16, { slider, status }),
                indeterminate,
            }),
        },
        {
            "不确定模式",
            ".IsIndeterminate(true) 后显示往复滚动的发光条，适合耗时未知的任务。",
            Column(12, {
                MakeStretchBar(4.0f),
                MakeStretchBar(8.0f),
                MakeStretchBar(14.0f),
            }),
        },
    };
    spec.source =
        "auto bar = ProgressBarWidget(40.0f, false);\n"
        "bar.Value(65.0f);          // 确定进度\n"
        "bar.IsIndeterminate(true); // 不确定模式\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery




