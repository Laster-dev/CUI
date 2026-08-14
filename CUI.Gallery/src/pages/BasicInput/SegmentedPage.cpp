#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/SegmentedControl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildSegmentedControlPage() {
    auto range = std::make_shared<SegmentedControl>();
    range->SetWidth(280.0f);
    range->AddItem("日");
    range->AddItem("周");
    range->AddItem("月");
    auto status = MakeStatus("");
    range->OnSelectionChanged().Connect([status](SegmentedControl*, int, const std::string& item) {
        status->SetText("正在显示：" + item + "。");
    });
    range->SetSelectedIndex(1);

    SamplePageSpec spec;
    spec.title = "SegmentedControl(分段控件)";
    spec.subtitle = "紧凑的互斥选择。选择模型与 ComboBox 相同，但没有下拉列表。";
    spec.sections = {
        {
            "日期范围",
            "日、周或月。单击某一段，或在聚焦时使用方向键。",
            Column(10).Children({ range, status }).Build(),
        },
    };
    spec.source =
        "auto range = std::make_shared<SegmentedControl>();\n"
        "range->AddItem(\"Day\");\n"
        "range->AddItem(\"Week\");\n"
        "range->AddItem(\"Month\");\n"
        "range->OnSelectionChanged().Connect([](SegmentedControl*, int, const std::string& item) {\n"
        "    // switch view\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
