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

std::shared_ptr<CUI::Button> MakeChip(const std::string& text, float width) {
    return ElevatedButton(text)
        .Background(Rgb(0x007ACC))
        .Width(width)
        .Height(32)
        .Build();
}

} // namespace

std::shared_ptr<UIElement> BuildWrapPanelPage() {
    // —— 水平换行 ——
    auto horizontal = Widgets::WrapPanel().Gap(10.0f).Shared();
    horizontal->SetOrientation(Orientation::Horizontal);
    const float widths[] = { 64, 96, 120, 76, 140, 88, 104, 128, 72, 92, 116, 84 };
    for (int i = 0; i < 12; ++i) {
                horizontal->AddChild(MakeChip(std::format("项目 {}", i + 1), widths[i]));
    }

    // —— 垂直换列（统一规格）——
    auto vertical = Widgets::WrapPanel().Shared();
    vertical->SetOrientation(Orientation::Vertical);
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
    auto liveWrap = Widgets::WrapPanel().Gap(10.0f).Shared();
    liveWrap->SetOrientation(Orientation::Horizontal);
    for (int i = 0; i < 10; ++i) {
                liveWrap->AddChild(MakeChip(std::format("标签 {}", i + 1), widths[i % 12]));
    }

    auto justified = Widgets::WrapPanel().Gap(10).Justified().FillLastLine(true).Shared();
    justified->SetOrientation(Orientation::Horizontal);
    const char* labels[] = { "Auto", "布局", "最小 72", "最大 180", "FlexGrow", "自动回流", "填满整行", "约束" };
    for (int i = 0; i < 8; ++i) {
        auto chip = ElevatedButton(labels[i]).Background(colors[i % 8]).Padding(14, 8, 14, 8)
            .MinWidth(72.0f)
            .MaxWidth(180.0f);
                chip->SetFlexGrow(i % 3 == 0 ? 2.0f : 1.0f);
                justified->AddChild(chip);
    }

    auto widthStatus = MakeStatus("宽度由卡片可用空间决定；缩放窗口可观察自动换行。");

    SamplePageSpec spec;
    spec.title = "WrapPanel(换行面板)";
    spec.subtitle = "按顺序排列子元素，超出可用空间时自动换行或换列，类似流式排版。";
    spec.sections = {
        {
            "水平换行",
            "Widgets::WrapPanel(\"Horizontal\")：子元素宽度不一，放不下一行时自动折行。",
            Column(12, {
                horizontal,
                MakeStatus("宽度跟随卡片可用空间；缩放窗口时 12 个不同宽度的按钮自动重新换行。"),
            }),
        },
        {
            "垂直换列（统一规格）",
            ".Orientation(Vertical) 配合 SetItemWidth / SetItemHeight 让所有子项等宽等高，超出高度时换到下一列。",
            Column(12, {
                vertical,
                MakeStatus("纵向模式按可用高度排列；在受限宿主中超出后自动换到下一列。"),
            }),
        },
        {
            "宽度变化实时重排",
            "父容器宽度变化时，子元素立即重新流式排布。",
            Column(12, {
                liveWrap,
                widthStatus,
            }),
        },
        {
            "Justified 流式填满",
            "JustifyLines 会将每一行的剩余空间按 FlexGrow 权重分配给子项；触及 MaxWidth 后不再拉伸，窗口缩小时先缩至 MinWidth 再换行。",
            Column(12, {
                justified,
                MakeStatus("默认最后一行同样填满；设置 FillLastLine(false) 可保留最后一行的自然宽度。"),
            }),
        },
    };
    spec.source =
        "auto panel = Widgets::WrapPanel(\"Horizontal\").Shared();\n"
        "// 宽度由父容器的可用空间决定。\n"
        "panel->Gap = 10.0f;\n"
        "panel->AddChild(ElevatedButton(\"项目 1\").Width(64).Height(32).Shared());\n"
        "\n"
        "// 垂直换列 + 统一规格\n"
        "auto v = Widgets::WrapPanel(\"Vertical\").Shared();\n"
        "// 受限高度下，Vertical 模式会自动换列。\n"
        "v->ItemWidth = 88.0f;\n"
        "v->ItemHeight = 34.0f;\n"
        "\n"
        "auto justified = Widgets::WrapPanel(\"Horizontal\").Justified().Shared();\n"
        "item.MinWidth = 72.0f;\n"
        "item.MaxWidth = 180.0f;\n"
        "item.FlexGrow = 1.0f;\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery



