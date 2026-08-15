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
    horizontal->Gap = 10.0f;
    const float widths[] = { 64, 96, 120, 76, 140, 88, 104, 128, 72, 92, 116, 84 };
    for (int i = 0; i < 12; ++i) {
        horizontal->AddChild(MakeChip(std::format("项目 {}", i + 1), widths[i]));
    }

    // —— 垂直换列（统一规格）——
    auto vertical = WrapPanelWidget("Vertical").Build();
    vertical->ItemWidth = 88.0f;
    vertical->ItemHeight = 34.0f;
    vertical->Gap = 8.0f;
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
    liveWrap->Gap = 10.0f;
    for (int i = 0; i < 10; ++i) {
        liveWrap->AddChild(MakeChip(std::format("标签 {}", i + 1), widths[i % 12]));
    }

    auto justified = WrapPanelWidget("Horizontal").Gap(10).Justified().FillLastLine().Build();
    const char* labels[] = { "Auto", "布局", "最小 72", "最大 180", "FlexGrow", "自动回流", "填满整行", "约束" };
    for (int i = 0; i < 8; ++i) {
        auto chip = ElevatedButton(labels[i]).Background(colors[i % 8]).Padding(14, 8, 14, 8).Build();
        chip->MinWidth = 72.0f;
        chip->MaxWidth = 180.0f;
        chip->FlexGrow = i % 3 == 0 ? 2.0f : 1.0f;
        justified->AddChild(chip);
    }

    auto widthStatus = MakeStatus("宽度由卡片可用空间决定；缩放窗口可观察自动换行。");

    SamplePageSpec spec;
    spec.title = "WrapPanel(换行面板)";
    spec.subtitle = "按顺序排列子元素，超出可用空间时自动换行或换列，类似流式排版。";
    spec.sections = {
        {
            "水平换行",
            "WrapPanelWidget(\"Horizontal\")：子元素宽度不一，放不下一行时自动折行。",
            Column(12).Children({
                horizontal,
                MakeStatus("宽度跟随卡片可用空间；缩放窗口时 12 个不同宽度的按钮自动重新换行。"),
            }).Build(),
        },
        {
            "垂直换列（统一规格）",
            "SetOrientation(Vertical) 配合 SetItemWidth / SetItemHeight 让所有子项等宽等高，超出高度时换到下一列。",
            Column(12).Children({
                vertical,
                MakeStatus("纵向模式按可用高度排列；在受限宿主中超出后自动换到下一列。"),
            }).Build(),
        },
        {
            "宽度变化实时重排",
            "父容器宽度变化时，子元素立即重新流式排布。",
            Column(12).Children({
                liveWrap,
                widthStatus,
            }).Build(),
        },
        {
            "Justified 流式填满",
            "JustifyLines 会将每一行的剩余空间按 FlexGrow 权重分配给子项；触及 MaxWidth 后不再拉伸，窗口缩小时先缩至 MinWidth 再换行。",
            Column(12).Children({
                justified,
                MakeStatus("默认最后一行同样填满；设置 FillLastLine(false) 可保留最后一行的自然宽度。"),
            }).Build(),
        },
    };
    spec.source =
        "auto panel = WrapPanelWidget(\"Horizontal\").Build();\n"
        "// 宽度由父容器的可用空间决定。\n"
        "panel->Gap = 10.0f;\n"
        "panel->AddChild(ElevatedButton(\"项目 1\").Width(64).Height(32).Build());\n"
        "\n"
        "// 垂直换列 + 统一规格\n"
        "auto v = WrapPanelWidget(\"Vertical\").Build();\n"
        "// 受限高度下，Vertical 模式会自动换列。\n"
        "v->ItemWidth = 88.0f;\n"
        "v->ItemHeight = 34.0f;\n"
        "\n"
        "auto justified = WrapPanelWidget(\"Horizontal\").Justified().Build();\n"
        "item.MinWidth = 72.0f;\n"
        "item.MaxWidth = 180.0f;\n"
        "item.FlexGrow = 1.0f;\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
