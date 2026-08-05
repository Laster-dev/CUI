#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildStackPanelPage(const ShowcaseContext&) {
    auto target = Row(12).Width(420).Height(220).Build();
    target->AddChild(ElevatedButton("弹性元素 #1").Background("#007ACC").Padding(14, 8, 14, 8).Build());
    target->AddChild(ElevatedButton("弹性元素 #2").Background("#10B981").Padding(14, 8, 14, 8).Build());
    target->AddChild(ElevatedButton("弹性元素 #3").Background("#D13438").Padding(14, 8, 14, 8).Build());
    auto combo = std::make_shared<ComboBox>();
    combo->AddItem("Horizontal");
    combo->AddItem("Vertical");
    combo->SetSelectedIndex(0);
    combo->SetWidth(280.0f);
    return { "StackPanel 线性布局", CreatePage(
        "StackPanel 线性布局控制台",
        "实时修改布局方向 (Orientation) 与 元素间距 (Gap)。",
        CreateDemoSurface({ target }, 0.0f),
        CreateRightPanel({
            CreateShowcaseText("线性面板属性表 (StackPanel)", 12.0f, "#569CD6", true),
            CreateShowcaseText("布局方向 (Orientation):", 11.0f, "#AAAAAA"),
            combo,
            CreateShowcaseText("元素间距 (Gap) [px]:", 11.0f, "#AAAAAA"),
            TextField("12").Width(280).Height(26).Build()
        })) };
}
