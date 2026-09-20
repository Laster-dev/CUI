#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildStackPanelPage(const ShowcaseContext&) {
    auto target = Row(12).Width(420).Height(220).Build();
        target->AddChild(ElevatedButton("弹性元素 #1").Background(Rgb(0x007ACC)).Padding(14, 8, 14, 8).Build());
        target->AddChild(ElevatedButton("弹性元素 #2").Background(Rgb(0x10B981)).Padding(14, 8, 14, 8).Build());
        target->AddChild(ElevatedButton("弹性元素 #3").Background(Rgb(0xD13438)).Padding(14, 8, 14, 8).Build());
    CUI::Widgets::Ref combo = CUI::Widgets::ComboBox().Shared();
    combo->AddItem("Horizontal");
    combo->AddItem("Vertical");
        combo.SelectedIndex(0);
        combo.Width(280.0f);
    return { "StackPanel 线性布局", CreatePage(
        "StackPanel 线性布局控制台",
        "实时修改布局方向 (Orientation) 与 元素间距 (Gap)。",
        CreateDemoSurface({ target }, 0.0f)) };
}
