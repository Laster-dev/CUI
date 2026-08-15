#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/core/Value.h"
#include "framework/controls/Slider.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<Button> MakeChip(const std::string& text, float width) {
    return ElevatedButton(text)
        .Background(Rgb(0x007ACC))
        .Width(width)
        .Height(32)
        .Build();
}

} // namespace

std::shared_ptr<UIElement> BuildWrapPanelPage() {
    // —— 水平换行 ——
    auto horizontal = WrapPanelWidget("Horizontal").Build();
    horizontal->SetWidth(540.0f);
    horizontal->SetGap(10.0f);
    const float widths[] = { 64, 96, 120, 76, 140, 88, 104, 128, 72, 92, 116, 84 };
    for (int i = 0; i < 12; ++i) {
        horizontal->AddChild(MakeChip(std::format("项目 {}", i + 1), widths[i]));
    }

    // —— 垂直换列（统一规格）——
    auto vertical = WrapPanelWidget("Vertical").Build();
    vertical->SetWidth(400.0f);
    vertical->SetHeight(200.0f);
    vertical->SetItemWidth(88.0f);
    vertical->SetItemHeight(34.0f);
    vertical->SetGap(8.0f);
    const D2D1_COLOR_F colors[] = {
        Rgb(0x007ACC), Rgb(0x0E639C), Rgb(0x10B981), Rgb(0xD13438),
        Rgb(0x845EF7), Rgb(0xF783AC), Rgb(0x22B8CF), Rgb(0xF59F00),
    };
    for (int i = 0; i < 12; ++i) {
        auto c = ElevatedButton(std::format("项 {}", i + 1))
            .Background(colors[i % 8])
            .Width(88.0f)
            .Height(34.0f)
            .Build();
        vertical->AddChild(c);
    }

    // —— 宽度变化实时重排 ——
    auto liveWrap = WrapPanelWidget("Horizontal").Build();
    liveWrap->SetWidth(540.0f);
    liveWrap->SetGap(10.0f);
    for (int i = 0; i < 10; ++i) {
        liveWrap->AddChild(MakeChip(std::format("标签 {}", i + 1), widths[i % 12]));
    }

    auto widthSlider = SliderWidget(540.0f, 300.0f, 640.0f).Width(260).Build();
    State<float> wrapWidth{ 540.0f };
    widthSlider->ValueProperty->Bind(wrapWidth);
    widthSlider->OnValueChanged().Connect([liveWrap](Slider* s, float) {
        liveWrap->SetWidth(s->GetValue());
    });

    auto widthStatusValue = MakeComputed<std::string>([](float w) {
        return std::format("面板宽度：{:.0f}px，拖动滑块观察自动换行", w);
    }, wrapWidth);
    auto widthStatus = MakeStatus("");
    widthStatus->Text->Bind(widthStatusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "WrapPanel(换行面板)";
    spec.subtitle = "按顺序排列子元素，超出可用空间时自动换行或换列，类似流式排版。";
    spec.sections = {
        {
            "水平换行",
            "WrapPanelWidget(\"Horizontal\")：子元素宽度不一，放不下一行时自动折行。",
            Column(12).Children({
                horizontal,
                MakeStatus("540px 宽度内 12 个不同宽度的按钮自动排成多行。"),
            }).Build(),
        },
        {
            "垂直换列（统一规格）",
            "SetOrientation(Vertical) 配合 SetItemWidth / SetItemHeight 让所有子项等宽等高，超出高度时换到下一列。",
            Column(12).Children({
                vertical,
                MakeStatus("200px 高度内纵向排布，超过后自动换列。"),
            }).Build(),
        },
        {
            "宽度变化实时重排",
            "在运行时修改 WrapPanel 宽度，子元素立即重新流式排布。",
            Column(12).Children({
                liveWrap,
                Row(16).Children({ widthSlider, widthStatus }).Build(),
            }).Build(),
        },
    };
    spec.source =
        "auto panel = WrapPanelWidget(\"Horizontal\").Build();\n"
        "panel->SetWidth(540.0f);\n"
        "panel->SetGap(10.0f);\n"
        "panel->AddChild(ElevatedButton(\"项目 1\").Width(64).Height(32).Build());\n"
        "\n"
        "// 垂直换列 + 统一规格\n"
        "auto v = WrapPanelWidget(\"Vertical\").Build();\n"
        "v->SetHeight(200.0f);\n"
        "v->SetItemWidth(88.0f);\n"
        "v->SetItemHeight(34.0f);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
