#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildGridPage(const ShowcaseContext&) {
    CUI::Widgets::Ref target = CUI::Widgets::Grid().Shared();
        target.Width(460.0f);
        target.Height(240.0f);
        target.ColumnDefinitions("1*,2*,100");
    target.RowDefinitions("40,1*,1*");
    target->AddChild(ElevatedButton("Cell(0,0)").Background(Rgb(0x007ACC)).GridRow(0).GridColumn(0).Build());
    target->AddChild(ElevatedButton("Cell(0,1) 2*").Background(Rgb(0x0E639C)).GridRow(0).GridColumn(1).Build());
    target->AddChild(ElevatedButton("100px").Background(Rgb(0x10B981)).GridRow(0).GridColumn(2).Build());
    target->AddChild(ElevatedButton("Row 1 Span 2 Cols").Background(Rgb(0xD13438)).GridRow(1).GridColumn(0).GridColumnSpan(2).Build());
    return { "Grid 网格布局", CreatePage(
        "Grid 网格布局属性控制台",
        "修改列定义 (ColumnDefinitions) 与 行定义 (RowDefinitions) 实时切割网格。",
        CreateDemoSurface({ target }, 0.0f)) };
}
