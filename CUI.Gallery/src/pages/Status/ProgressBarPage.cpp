#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<ProgressBar> MakeStretchBar(float height) {
    CUI::Widgets::Ref bar = Widgets::ProgressBar().Shared();
    bar.Value(0.0f);
    bar.IsIndeterminate(true);
        bar.Align(Alignment::Stretch);
        bar.Height(height);
    return bar;
}

} // namespace

Element BuildProgressBarPage() {
    // 确定进度：滑块驱动填充值。
    CUI::Widgets::Ref determinate = Widgets::ProgressBar().Shared();
    determinate.Value(40.0f);
    determinate.IsIndeterminate(false);
        determinate.Align(Alignment::Stretch);
        determinate.Height(6.0f);

    State<float> value{ 40.0f };
    CUI::Widgets::Ref slider = Widgets::Slider().Shared();
    slider.Minimum(0.0f);
    slider.Maximum(100.0f);
    slider.Value(40.0f);
        slider.Step(1.0f);
        slider.Width(280.0f);
    slider->ValueProperty.Bind(value);

    auto bar = determinate;
    value.OnChanged().Connect([bar](const float& v) {
                bar.Value(v);
    });

    auto statusValue = MakeComputed<std::string>([](float v) {
        return std::format("进度：{:.0f}%", v);
    }, value);
    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    CUI::Widgets::Ref indeterminate = Widgets::ToggleSwitch().Header("不确定模式").IsOn(false).Shared();
    indeterminate->OnToggled().Connect([bar](ToggleSwitch*, bool on) {
                bar.IsIndeterminate(on);
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
        "auto bar = Widgets::ProgressBar(40.0f, false).Shared();\n"
        "bar.Value(65.0f);          // 确定进度\n"
        "bar.IsIndeterminate(true); // 不确定模式\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery




