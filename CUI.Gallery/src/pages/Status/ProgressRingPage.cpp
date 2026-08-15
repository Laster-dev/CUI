#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<ProgressRing> MakeRing(float size, float value, bool indeterminate) {
    auto ring = ProgressRingWidget(value, indeterminate);
    ring->SetWidth(size);
    ring->SetHeight(size);
    return ring;
}

} // namespace

Element BuildProgressRingPage() {
    // 确定进度：滑块驱动弧线增长。
    auto determinate = ProgressRingWidget(60.0f, false);
    determinate->SetWidth(96.0f);
    determinate->SetHeight(96.0f);

    State<float> value{ 60.0f };
    auto slider = SliderWidget(60.0f, 0.0f, 100.0f);
    slider->SetStep(1.0f);
    slider->Width = 280.0f;
    slider->ValueProperty.Bind(value);

    auto ring = determinate;
    value.OnChanged().Connect([ring](const float& v) {
        ring->SetValue(v);
    });

    auto statusValue = MakeComputed<std::string>([](float v) {
        return std::format("进度：{:.0f}%", v);
    }, value);
    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto indeterminate = ToggleSwitchTile("不确定模式", false);
    indeterminate->OnToggled().Connect([ring](ToggleSwitch*, bool on) {
        ring->SetIsIndeterminate(on);
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
            "SetIsIndeterminate(true) 后弧线不断追逐旋转，用于等待场景。",
            Row(24, {
                MakeRing(24.0f, 0.0f, true),
                MakeRing(48.0f, 0.0f, true),
                MakeRing(80.0f, 0.0f, true),
                MakeRing(120.0f, 0.0f, true),
            }),
        },
    };
    spec.source =
        "auto ring = ProgressRingWidget(60.0f, false);\n"
        "ring->SetWidth(96.0f);\n"
        "ring->SetHeight(96.0f);\n"
        "ring->SetValue(75.0f);          // 确定进度\n"
        "ring->SetIsIndeterminate(true); // 不确定模式\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
