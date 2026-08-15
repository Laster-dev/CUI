#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/SegmentedControl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildSegmentedControlPage() {
    auto range = Make<SegmentedControl>();
    range->SetWidth(280.0f);
    range->AddItem("日");
    range->AddItem("周");
    range->AddItem("月");

    State<int> selection{ 1 };
    range->SelectedIndex.Bind(selection);

    auto statusValue = MakeComputed<std::string>([](int index) {
        switch (index) {
        case 0: return "正在显示：日。";
        case 1: return "正在显示：周。";
        case 2: return "正在显示：月。";
        default: return "选择日期范围。";
        }
    }, selection);

    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "SegmentedControl(分段控件)";
    spec.subtitle = "紧凑的互斥选择。通过状态绑定同步当前段。";
    spec.sections = {
        {
            "日期范围",
            "日、周或月。单击某一段，或在聚焦时使用方向键。",
            Column(10).Children({ range, status }).Build(),
        },
    };
    spec.source =
        "State<int> selection{ 1 };\n"
        "range->SelectedIndex.Bind(selection);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
