#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildWrapPage(const ShowcaseContext&) {
    auto target = std::make_shared<WrapPanel>();
    CUI::DSL::Borrow(target).Width(380.0f);
    CUI::DSL::Borrow(target).Height(240.0f);
    CUI::DSL::Borrow(target).Orientation(Orientation::Horizontal);
    for (int i = 1; i <= 6; ++i) {
        auto chip = ElevatedButton("Tag #" + std::to_string(i)).Background(Rgb(0x007ACC)).Padding(14, 6, 14, 6).Build();
        CUI::DSL::Borrow(chip).Margin(Thickness(4));
        CUI::DSL::Borrow(target).AddChild(chip);
    }
    auto combo = std::make_shared<ComboBox>();
    combo->AddItem("Horizontal");
    combo->AddItem("Vertical");
    CUI::DSL::Borrow(combo).SelectedIndex(0);
    CUI::DSL::Borrow(combo).Width(280.0f);
    return { "WrapPanel 流式布局", CreatePage(
        "WrapPanel 流式布局控制台",
        "多行流式排版，达到边缘时自动折行或折列。",
        CreateDemoSurface({ target }, 0.0f)) };
}
