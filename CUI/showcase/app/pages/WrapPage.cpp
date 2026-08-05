#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildWrapPage(const ShowcaseContext&) {
    auto target = std::make_shared<WrapPanel>();
    target->SetWidth(380.0f);
    target->SetHeight(240.0f);
    target->SetOrientation(Orientation::Horizontal);
    for (int i = 1; i <= 6; ++i) {
        auto chip = ElevatedButton("Tag #" + std::to_string(i)).Background("#007ACC").Padding(14, 6, 14, 6).Build();
        chip->SetMargin(Thickness(4));
        target->AddChild(chip);
    }
    auto combo = std::make_shared<ComboBox>();
    combo->AddItem("Horizontal");
    combo->AddItem("Vertical");
    combo->SetSelectedIndex(0);
    combo->SetWidth(280.0f);
    return { "WrapPanel 流式布局", CreatePage(
        "WrapPanel 流式布局控制台",
        "多行流式排版，达到边缘时自动折行或折列。",
        CreateDemoSurface({ target }, 0.0f),
        CreateRightPanel({
            CreateShowcaseText("流式面板属性表 (WrapPanel)", 12.0f, "#569CD6", true),
            CreateShowcaseText("排版方向 (Orientation):", 11.0f, "#AAAAAA"),
            combo
        })) };
}
