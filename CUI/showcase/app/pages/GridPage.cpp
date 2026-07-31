#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildGridPage(const ShowcaseContext&) {
    auto target = std::make_shared<Grid>();
    target->SetProperty("width", Value(460.0f));
    target->SetProperty("height", Value(240.0f));
    target->SetColumnDefinitions("1*,2*,100");
    target->SetRowDefinitions("40,1*,1*");
    auto a = ElevatedButton("Cell(0,0)").Background("#007ACC").Build(); a->SetProperty("Grid.Row", Value(0)); a->SetProperty("Grid.Column", Value(0));
    auto b = ElevatedButton("Cell(0,1) 2*").Background("#0E639C").Build(); b->SetProperty("Grid.Row", Value(0)); b->SetProperty("Grid.Column", Value(1));
    auto c = ElevatedButton("100px").Background("#10B981").Build(); c->SetProperty("Grid.Row", Value(0)); c->SetProperty("Grid.Column", Value(2));
    auto d = ElevatedButton("Row 1 Span 2 Cols").Background("#D13438").Build(); d->SetProperty("Grid.Row", Value(1)); d->SetProperty("Grid.Column", Value(0)); d->SetProperty("Grid.ColumnSpan", Value(2));
    target->AddChild(a); target->AddChild(b); target->AddChild(c); target->AddChild(d);
    return { "Grid 网格布局", CreatePage(
        "Grid 网格布局属性控制台",
        "修改列定义 (ColumnDefinitions) 与 行定义 (RowDefinitions) 实时切割网格。",
        CreateDemoSurface({ target }, 0.0f),
        CreateRightPanel({
            CreateShowcaseText("网格属性控制表 (Grid)", 12.0f, "#569CD6", true),
            CreateShowcaseText("列定义 (ColumnDefinitions):", 11.0f, "#AAAAAA"),
            TextField("1*,2*,100").Width(280).Height(26).Build(),
            CreateShowcaseText("行定义 (RowDefinitions):", 11.0f, "#AAAAAA"),
            TextField("40,1*,1*").Width(280).Height(26).Build()
        })) };
}
