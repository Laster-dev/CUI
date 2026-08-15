#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/core/Value.h"
#include "framework/controls/ComboBox.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<Button> MakeCell(
    const std::string& text,
    D2D1_COLOR_F color,
    int row,
    int column,
    int columnSpan = 1,
    int rowSpan = 1) {
    auto cell = ElevatedButton(text).Background(color).Padding(10, 8, 10, 8).Build();
    cell->SetGridRow(row);
    cell->SetGridColumn(column);
    if (columnSpan > 1) cell->SetGridColumnSpan(columnSpan);
    if (rowSpan > 1) cell->SetGridRowSpan(rowSpan);
    return cell;
}

} // namespace

std::shared_ptr<UIElement> BuildGridPage() {
    auto proportionalGrid = Make<Grid>();
    proportionalGrid->SetColumnDefinitions("1*,2*,100");
    proportionalGrid->SetRowDefinitions("Auto,Auto,Auto");
    proportionalGrid->AddChild(MakeCell("第 1 列 · 1*", Rgb(0x007ACC), 0, 0));
    proportionalGrid->AddChild(MakeCell("第 2 列 · 2*", Rgb(0x0E639C), 0, 1));
    proportionalGrid->AddChild(MakeCell("100px", Rgb(0x10B981), 0, 2));
    proportionalGrid->AddChild(MakeCell("跨越前两列", Rgb(0xD13438), 1, 0, 2));
    proportionalGrid->AddChild(MakeCell("第 3 列", Rgb(0xF783AC), 1, 2));
    proportionalGrid->AddChild(MakeCell("每行由内容高度决定", Rgb(0x845EF7), 2, 0, 3));

    auto spanGrid = Make<Grid>();
    spanGrid->SetColumnDefinitions("1*,1*,1*");
    spanGrid->SetRowDefinitions("Auto,Auto,Auto");
    spanGrid->AddChild(MakeCell("横向跨 2 列", Rgb(0x007ACC), 0, 0, 2));
    spanGrid->AddChild(MakeCell("右上", Rgb(0x10B981), 0, 2));
    spanGrid->AddChild(MakeCell("左下", Rgb(0xD13438), 1, 0));
    spanGrid->AddChild(MakeCell("纵向跨 2 行", Rgb(0x0E639C), 1, 1, 1, 2));
    spanGrid->AddChild(MakeCell("右中", Rgb(0x845EF7), 1, 2));
    spanGrid->AddChild(MakeCell("底部", Rgb(0xF783AC), 2, 0));
    spanGrid->AddChild(MakeCell("右下", Rgb(0x22B8CF), 2, 2));

    auto liveGrid = Make<Grid>();
    auto cellA = MakeCell("A", Rgb(0x007ACC), 0, 0);
    auto cellB = MakeCell("B", Rgb(0x10B981), 0, 1);
    auto cellC = MakeCell("C", Rgb(0xD13438), 0, 2);
    liveGrid->AddChild(cellA);
    liveGrid->AddChild(cellB);
    liveGrid->AddChild(cellC);
    liveGrid->SetColumnDefinitions("1*,1*,1*");
    liveGrid->SetRowDefinitions("Auto");

    auto layoutCombo = Make<ComboBox>();
    layoutCombo->AddItem("三列均分（1*, 1*, 1*）");
    layoutCombo->AddItem("左窄右宽（1*, 2*）");
    layoutCombo->AddItem("右窄左宽（2*, 1*）");
    layoutCombo->SetSelectedIndex(0);

    State<int> layoutIndex{ 0 };
    layoutCombo->SelectedIndex.Bind(layoutIndex);

    auto applyPreset = [liveGrid, cellA, cellB, cellC](int index) {
        if (index == 0) {
            liveGrid->SetColumnDefinitions("1*,1*,1*");
            liveGrid->SetRowDefinitions("Auto");
            cellA->SetGridRow(0); cellA->SetGridColumn(0); cellA->SetGridColumnSpan(1);
            cellB->SetGridRow(0); cellB->SetGridColumn(1); cellB->SetGridColumnSpan(1);
            cellC->SetGridRow(0); cellC->SetGridColumn(2); cellC->SetGridColumnSpan(1);
            return;
        }

        liveGrid->SetColumnDefinitions(index == 1 ? "1*,2*" : "2*,1*");
        liveGrid->SetRowDefinitions("Auto,Auto");
        cellA->SetGridRow(0); cellA->SetGridColumn(0); cellA->SetGridColumnSpan(1);
        cellB->SetGridRow(0); cellB->SetGridColumn(1); cellB->SetGridColumnSpan(1);
        cellC->SetGridRow(1); cellC->SetGridColumn(0); cellC->SetGridColumnSpan(2);
    };

    layoutCombo->OnSelectionChanged().Connect([applyPreset](ComboBox*, int index, const std::string&) {
        applyPreset(index);
    });

    auto layoutStatus = MakeStatus("三列均分：A、B、C 各占一列。");
    layoutStatus->Text.Bind(MakeComputed<std::string>([](int index) {
        if (index == 0) return std::string("三列均分：A、B、C 各占一列。");
        if (index == 1) return std::string("左窄右宽：C 位于第二行并横跨全部列。");
        return std::string("右窄左宽：C 位于第二行并横跨全部列。");
    }, layoutIndex), BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "Grid(网格)";
    spec.subtitle = "使用行列定义组织元素。页面的宽度由父级约束，因此列可以使用星号比例；页面高度由内容决定，因此行使用 Auto。";
    spec.sections = {
        {
            "比例列与内容行",
            "列定义 1*,2*,100 按可用宽度分配；Auto 行只取内容所需高度，不会在纵向滚动页面中占用无穷高度。",
            Column(12).Children({
                proportionalGrid,
                MakeStatus("页面内容在垂直 ScrollViewer 中测量，行使用 Auto；需要固定可用高度的场景才使用 * 行。"),
            }).Build(),
        },
        {
            "跨行与跨列",
            "Grid.ColumnSpan 和 Grid.RowSpan 合并相邻单元格；其他单元格仍按自己的行列位置排列。",
            Column(12).Children({
                spanGrid,
                MakeStatus("蓝色单元格横跨两列，深蓝单元格纵向跨越两行。"),
            }).Build(),
        },
        {
            "运行时切换列定义",
            "切换列比例和单元格位置后，Grid 会立即重新测量和排列；不依赖窗口缩放。",
            Column(12).Children({
                liveGrid,
                WrapPanelWidget("Horizontal").Gap(12).Children({ layoutCombo, layoutStatus }).Build(),
            }).Build(),
        },
    };
    spec.source =
        "auto grid = Make<Grid>();\n"
        "grid->SetColumnDefinitions(\"1*,2*,100\");\n"
        "grid->SetRowDefinitions(\"Auto,Auto,Auto\");\n"
        "// 宽度由父容器提供，因此 * 列按比例分配。\n"
        "// 页面垂直滚动时高度无界，因此使用 Auto 行。\n"
        "\n"
        "cell->SetGridRow(1);\n"
        "cell->SetGridColumn(0);\n"
        "cell->SetGridColumnSpan(2);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
