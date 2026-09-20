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

std::shared_ptr<ProgressRing> MakeRing(float size, float value, bool indeterminate) {
    CUI::Widgets::Ref ring = Widgets::ProgressRing().Shared();
    ring.Value(value);
    ring.IsIndeterminate(indeterminate);
        ring.Width(size);
        ring.Height(size);
    return ring;
}

} // namespace

Element BuildProgressRingPage() {
    // 确定进度：滑块驱动弧线增长。
    CUI::Widgets::Ref determinate = Widgets::ProgressRing().Shared();
    determinate.Value(60.0f);
    determinate.IsIndeterminate(false);
        determinate.Width(96.0f);
        determinate.Height(96.0f);

    State<float> value{ 60.0f };
    CUI::Widgets::Ref slider = Widgets::Slider().Shared();
    slider.Minimum(0.0f);
    slider.Maximum(100.0f);
    slider.Value(60.0f);
        slider.Step(1.0f);
        slider.Width(280.0f);
    slider->ValueProperty.Bind(value);

    auto ring = determinate;
    value.OnChanged().Connect([ring](const float& v) {
                ring.Value(v);
    });

    auto statusValue = MakeComputed<std::string>([](float v) {
        return std::format("进度：{:.0f}%", v);
    }, value);
    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    CUI::Widgets::Ref indeterminate = Widgets::ToggleSwitch().Header("不确定模式").IsOn(false).Shared();
    indeterminate->OnToggled().Connect([ring](ToggleSwitch*, bool on) {
                ring.IsIndeterminate(on);
    });

    SamplePageSpec spec;
    spec.title = "ProgressRing(进度环)";
    spec.subtitle = "指示耗时任务的环形加载控件，支持确定值与不确定两种模式。";
    spec.sections = {
        {
            "确定进度",
            "拖动滑块改变进度值；进度弧线从 12 点钟方向顺时针增长。",
            Row(24, {
                determinate,
                Column(12, { slider, status, indeterminate }),
            }),
        },
        {
            "不确定模式",
            ".IsIndeterminate(true) 后弧线不断追逐旋转，用于等待场景。",
            Row(24, {
                MakeRing(24.0f, 0.0f, true),
                MakeRing(48.0f, 0.0f, true),
                MakeRing(80.0f, 0.0f, true),
                MakeRing(120.0f, 0.0f, true),
            }),
        },
    };
    spec.source =
        "auto ring = Widgets::ProgressRing(60.0f, false).Shared();\n"
        "ring.Width(96.0f);\n"
        "ring.Height(96.0f);\n"
        "ring.Value(75.0f);          // 确定进度\n"
        "ring.IsIndeterminate(true); // 不确定模式\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery





