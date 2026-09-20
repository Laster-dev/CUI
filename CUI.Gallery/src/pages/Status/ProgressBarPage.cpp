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
    auto bar = Widgets::ProgressBar().Shared();
    bar->SetValue(0.0f);
    bar->SetIsIndeterminate(true);
        bar->SetAlign(Alignment::Stretch);
        bar->SetHeight(height);
    return bar;
}

} // namespace

Element BuildProgressBarPage() {
    // 确定进度：滑块驱动填充值。
    auto determinate = Widgets::ProgressBar().Shared();
    determinate->SetValue(40.0f);
    determinate->SetIsIndeterminate(false);
        determinate->SetAlign(Alignment::Stretch);
        determinate->SetHeight(6.0f);

    State<float> value{ 40.0f };
    auto slider = Widgets::Slider().Shared();
    slider->SetMinimum(0.0f);
    slider->SetMaximum(100.0f);
    slider->SetValue(40.0f);
        slider->SetStep(1.0f);
        slider->SetWidth(280.0f);
    slider->ValueProperty.Bind(value);

    auto bar = determinate;
    value.OnChanged().Connect([bar](const float& v) {
                bar->SetValue(v);
    });

    auto statusValue = MakeComputed<std::string>([](float v) {
        return std::format("进度：{:.0f}%", v);
    }, value);
    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto indeterminate = ToggleSwitchTile("不确定模式", false);
    indeterminate->OnToggled().Connect([bar](ToggleSwitch*, bool on) {
                bar->SetIsIndeterminate(on);
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




