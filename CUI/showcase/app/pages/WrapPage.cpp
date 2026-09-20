#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildWrapPage(const ShowcaseContext&) {
    CUI::Widgets::Ref target = CUI::Widgets::WrapPanel().Shared();
        target.Width(380.0f);
        target.Height(240.0f);
        target.Orientation(Orientation::Horizontal);
    for (int i = 1; i <= 6; ++i) {
        auto chip = ElevatedButton("Tag #" + std::to_string(i)).Background(Rgb(0x007ACC)).Padding(14, 6, 14, 6).Build();
                chip->SetMargin(Thickness(4));
                target->AddChild(chip);
    }
    CUI::Widgets::Ref combo = CUI::Widgets::ComboBox().Shared();
    combo->AddItem("Horizontal");
    combo->AddItem("Vertical");
        combo.SelectedIndex(0);
        combo.Width(280.0f);
    return { "WrapPanel 流式布局", CreatePage(
        "WrapPanel 流式布局控制台",
        "多行流式排版，达到边缘时自动折行或折列。",
        CreateDemoSurface({ target }, 0.0f)) };
}
