#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/core/Value.h"
#include "framework/controls/ComboBox.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<Button> MakeCell(
    const std::string& text,
    D2D1_COLOR_F color,
    int row,
    int col,
    int colSpan = 1,
    int rowSpan = 1) {
    auto b = ElevatedButton(text).Background(color).Padding(10, 8, 10, 8).Build();
    b->SetGridRow(row);
    b->SetGridColumn(col);
    if (colSpan > 1) b->SetGridColumnSpan(colSpan);
    if (rowSpan > 1) b->SetGridRowSpan(rowSpan);
    return b;
}

} // namespace

std::shared_ptr<UIElement> BuildGridPage() {
    // —— 常规用法：混合单位 ——
    auto grid = Make<Grid>();
    grid->SetWidth(540.0f);
    grid->SetHeight(180.0f);
    grid->SetColumnDefinitions("1*,2*,100");
    grid->SetRowDefinitions("40,1*,1*");
    grid->AddChild(MakeCell("Cell(0,0) · 1*", Rgb(0x007ACC), 0, 0));
    grid->AddChild(MakeCell("Cell(0,1) · 2*", Rgb(0x0E639C), 0, 1));
    grid->AddChild(MakeCell("100px 固定", Rgb(0x10B981), 0, 2));
    grid->AddChild(MakeCell("Row1 · 跨 2 列", Rgb(0xD13438), 1, 0, 2));
    grid->AddChild(MakeCell("Cell(2,0)", Rgb(0x845EF7), 2, 0));
    grid->AddChild(MakeCell("Cell(2,2)", Rgb(0xF783AC), 2, 2));

    // —— 跨行跨列 ——
    auto spanGrid = Make<Grid>();
    spanGrid->SetWidth(540.0f);
    spanGrid->SetHeight(180.0f);
    spanGrid->SetColumnDefinitions("1*,1*,1*");
    spanGrid->SetRowDefinitions("1*,1*,1*");
    spanGrid->AddChild(MakeCell("横向跨 2 列", Rgb(0x007ACC), 0, 0, 2));
    spanGrid->AddChild(MakeCell("纵向跨 2 行", Rgb(0x0E639C), 0, 2, 1, 2));
    spanGrid->AddChild(MakeCell("A", Rgb(0x10B981), 1, 0));
    spanGrid->AddChild(MakeCell("B", Rgb(0xD13438), 2, 0));
    spanGrid->AddChild(MakeCell("C · 跨 2 行", Rgb(0x845EF7), 1, 1, 1, 2));
    spanGrid->AddChild(MakeCell("D", Rgb(0xF783AC), 2, 2));

    // —— 运行时切换布局 ——
    auto liveGrid = Make<Grid>();
    liveGrid->SetWidth(540.0f);
    liveGrid->SetHeight(150.0f);

    auto cellA = MakeCell("A", Rgb(0x007ACC), 0, 0);
    auto cellB = MakeCell("B", Rgb(0x10B981), 0, 1);
    auto cellC = MakeCell("C", Rgb(0xD13438), 0, 2);
    liveGrid->AddChild(cellA);
    liveGrid->AddChild(cellB);
    liveGrid->AddChild(cellC);
    liveGrid->SetColumnDefinitions("1*,1*,1*");
    liveGrid->SetRowDefinitions("1*");

    auto layoutCombo = Make<ComboBox>();
    layoutCombo->SetWidth(300.0f);
    layoutCombo->AddItem("三列均分 (1*,1*,1*)");
    layoutCombo->AddItem("左窄右宽 (1*,2*)");
    layoutCombo->AddItem("右窄左宽 (2*,1*)");
    layoutCombo->SetSelectedIndex(0);

    State<int> layoutIndex{ 0 };
    layoutCombo->SelectedIndex->Bind(layoutIndex);

    auto applyPreset = [liveGrid, cellA, cellB, cellC](int index) {
        if (index == 0) {
            liveGrid->SetRowDefinitions("1*");
            liveGrid->SetColumnDefinitions("1*,1*,1*");
            cellA->SetGridRow(0); cellA->SetGridColumn(0); cellA->SetGridColumnSpan(1);
            cellB->SetGridRow(0); cellB->SetGridColumn(1); cellB->SetGridColumnSpan(1);
            cellC->SetGridRow(0); cellC->SetGridColumn(2); cellC->SetGridColumnSpan(1);
        } else if (index == 1) {
            liveGrid->SetRowDefinitions("1*,40");
            liveGrid->SetColumnDefinitions("1*,2*");
            cellA->SetGridRow(0); cellA->SetGridColumn(0); cellA->SetGridColumnSpan(1);
            cellB->SetGridRow(0); cellB->SetGridColumn(1); cellB->SetGridColumnSpan(1);
            cellC->SetGridRow(1); cellC->SetGridColumn(0); cellC->SetGridColumnSpan(2);
        } else {
            liveGrid->SetRowDefinitions("1*,40");
            liveGrid->SetColumnDefinitions("2*,1*");
            cellA->SetGridRow(0); cellA->SetGridColumn(0); cellA->SetGridColumnSpan(1);
            cellB->SetGridRow(0); cellB->SetGridColumn(1); cellB->SetGridColumnSpan(1);
            cellC->SetGridRow(1); cellC->SetGridColumn(0); cellC->SetGridColumnSpan(2);
        }
    };

    layoutCombo->OnSelectionChanged().Connect([applyPreset](ComboBox*, int index, const std::string&) {
        applyPreset(index);
    });

    auto layoutStatusValue = MakeComputed<std::string>([](int index) {
        switch (index) {
        case 0: return "当前布局：三列均分，A/B/C 各占一列。";
        case 1: return "当前布局：左窄右宽两列，C 横跨底部整行。";
        default: return "当前布局：右窄左宽两列，C 横跨底部整行。";
        }
    }, layoutIndex);
    auto layoutStatus = MakeStatus("");
    layoutStatus->Text->Bind(layoutStatusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "Grid(网格)";
    spec.subtitle = "按行和列组织元素的布局容器，支持固定像素、星号比例（*）与自动（Auto）三种列宽单位。";
    spec.sections = {
        {
            "常规用法",
            "SetColumnDefinitions(\"1*,2*,100\")：第 1 列占 1 份比例、第 2 列占 2 份比例、第 3 列固定 100px；首行固定 40px，其余两行均分。",
            Column(12).Children({
                grid,
                MakeStatus("1* 与 2* 按剩余空间比例分配，100 固定像素不参与伸缩。"),
            }).Build(),
        },
        {
            "跨行跨列",
            "通过 SetGridColumnSpan / SetGridRowSpan 合并单元格，子元素按 Grid.Row / Grid.Column 附加属性归位。",
            Column(12).Children({
                spanGrid,
                MakeStatus("蓝色横向跨 2 列；深蓝纵向跨 2 行；紫色跨 2 行。"),
            }).Build(),
        },
        {
            "运行时切换布局",
            "在运行时修改 SetColumnDefinitions / SetRowDefinitions 与子元素的行列位置，网格立即重排。",
            Column(12).Children({
                liveGrid,
                Row(12).Children({ layoutCombo, layoutStatus }).Build(),
            }).Build(),
        },
    };
    spec.source =
        "auto grid = Make<Grid>();\n"
        "grid->SetWidth(540.0f);\n"
        "grid->SetHeight(180.0f);\n"
        "grid->SetColumnDefinitions(\"1*,2*,100\");\n"
        "grid->SetRowDefinitions(\"40,1*,1*\");\n"
        "\n"
        "auto a = ElevatedButton(\"A\").Build();\n"
        "a->SetGridRow(0);\n"
        "a->SetGridColumn(0);\n"
        "grid->AddChild(a);\n"
        "\n"
        "auto b = ElevatedButton(\"B\").Build();\n"
        "b->SetGridRow(1);\n"
        "b->SetGridColumn(0);\n"
        "b->SetGridColumnSpan(2);   // 横向合并 2 列\n"
        "grid->AddChild(b);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
