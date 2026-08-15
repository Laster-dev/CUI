#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/core/Value.h"
#include "framework/controls/Slider.h"
#include <cmath>
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

const D2D1_COLOR_F kCellColors[] = {
    Rgb(0x007ACC), Rgb(0x0E639C), Rgb(0x10B981), Rgb(0xD13438),
    Rgb(0x845EF7), Rgb(0xF59F00), Rgb(0x22B8CF), Rgb(0xF783AC),
};

std::shared_ptr<CUI::Button> MakeCell(const std::string& text, int index) {
    return ElevatedButton(text).Background(kCellColors[index % 8]).Build();
}

} // namespace

std::shared_ptr<UIElement> BuildUniformGridPage() {
    // —— 固定行列 ——
    auto fixed = UniformGridWidget(2, 3).Width(520).Height(180).Build();
    for (int i = 0; i < 6; ++i) {
        fixed->AddChild(MakeCell(std::format("格 {}", i + 1), i));
    }

    // —— 自动计算行列 ——
    auto autoGrid = UniformGridWidget(0, 0).Width(520).Height(180).Build();
    for (int i = 0; i < 7; ++i) {
        autoGrid->AddChild(MakeCell(std::format("自动 {}", i + 1), i));
    }

    // —— 只指定列数 ——
    auto colsOnly = UniformGridWidget(0, 4).Width(520).Height(150).Build();
    for (int i = 0; i < 8; ++i) {
        colsOnly->AddChild(MakeCell(std::format("项 {}", i + 1), i));
    }

    // —— 运行时调整行列 ——
    auto liveGrid = UniformGridWidget(2, 3).Width(520).Height(220).Build();
    liveGrid->ClipToBounds = true;
    for (int i = 0; i < 8; ++i) {
        liveGrid->AddChild(MakeCell(std::format("格 {}", i + 1), i));
    }

    auto rowsSlider = SliderWidget(2.0f, 1.0f, 4.0f).Build();
    rowsSlider->FlexGrow = 1.0f;
    auto colsSlider = SliderWidget(3.0f, 1.0f, 5.0f).Build();
    colsSlider->FlexGrow = 1.0f;

    State<float> rowsValue{ 2.0f };
    State<float> colsValue{ 3.0f };
    rowsSlider->ValueProperty.Bind(rowsValue);
    colsSlider->ValueProperty.Bind(colsValue);
    // 滑块浮点值经转换器驱动 Rows/Columns 整数属性，双向绑定无需手动事件转发。
    liveGrid->Rows.Bind(rowsValue, MakeConverter<float, int>([](float v) {
        return static_cast<int>(std::lround(v));
    }), BindingMode::OneWay);
    liveGrid->Columns.Bind(colsValue, MakeConverter<float, int>([](float v) {
        return static_cast<int>(std::lround(v));
    }), BindingMode::OneWay);

    auto gridStatus = MakeStatus("");
    gridStatus->Text.Bind(MakeComputed<std::string>([](float r, float c) {
        return std::format("当前布局：{} 行 × {} 列 —— 所有单元格等宽等高，均分可用空间。",
                           static_cast<int>(std::lround(r)),
                           static_cast<int>(std::lround(c)));
    }, rowsValue, colsValue), BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "UniformGrid(等距网格)";
    spec.subtitle = "将可用空间均分为完全相等的单元格，子元素按行列依次填入。";
    spec.sections = {
        {
            "固定行列",
            "UniformGridWidget(rows, cols) 显式指定行列数，每个单元格严格等宽等高。",
            Column(12, {
                fixed,
                MakeStatus("2 行 × 3 列，6 个格子均分 520×180 的可用区域。"),
            }),
        },
        {
            "自动计算行列",
            "rows 与 cols 都传 0 时，根据子元素数量自动推导：先按 √n 取行数，再向上取整补足列数。",
            Column(12, {
                autoGrid,
                MakeStatus("7 个子元素自动推导为 3 行 × 3 列（√7≈2.65 → 3 行，7/3 → 3 列），右下两个格子留空。"),
            }),
        },
        {
            "只指定列数",
            "仅指定列数（rows = 0）时行数由子元素数量自动推导，常用于等宽九宫格等场景。",
            Column(12, {
                colsOnly,
                MakeStatus("4 列固定，8 个子元素自动排成 2 行。"),
            }),
        },
        {
            "运行时调整行列",
            "通过 Rows / Columns 属性在运行时改变行列数，网格立即重新等分；格子数不足时多余子元素被裁剪。",
            Column(12, {
                liveGrid,
                WrapPanelWidget("Horizontal").Gap(16).Children({
                    rowsSlider,
                    colsSlider,
                    gridStatus,
                }).Build(),
            }),
        },
    };
    spec.source =
        "auto grid = UniformGridWidget(2, 3).Width(520).Height(180).Build();\n"
        "grid->AddChild(ElevatedButton(\"格 1\").Build());\n"
        "// ... 共 6 个子元素，均分 2 行 × 3 列\n"
        "\n"
        "// 行列传 0 时自动推导：√n 行、按需补列\n"
        "auto autoGrid = UniformGridWidget(0, 0).Build();\n"
        "\n"
        "// 只指定列数，行数自动计算\n"
        "auto colsOnly = UniformGridWidget(0, 4).Build();\n"
        "\n"
        "// 运行时调整行列：状态经转换器驱动整数属性\n"
        "State<float> rowsState{ 2.0f };\n"
        "grid->Rows.Bind(rowsState, MakeConverter<float, int>(\n"
        "    [](float v) { return (int)std::lround(v); }), BindingMode::OneWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
