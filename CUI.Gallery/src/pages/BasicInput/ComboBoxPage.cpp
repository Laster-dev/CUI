#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/ComboBox.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildComboBoxPage() {
    auto combo = std::make_shared<ComboBox>();
    combo->SetWidth(220.0f);
    combo->AddItem("苹果");
    combo->AddItem("香蕉");
    combo->AddItem("樱桃");
    combo->AddItem("橙子");
    auto status = MakeStatus("请选择水果。");
    combo->OnSelectionChanged().Connect([status](ComboBox*, int, const std::string& item) {
        status->SetText("已选择：" + item + "。");
    });
    combo->SetSelectedIndex(0);

    auto disabled = std::make_shared<ComboBox>();
    disabled->SetWidth(220.0f);
    disabled->AddItem("不可用");
    disabled->SetSelectedIndex(0);
    disabled->SetIsEnabled(false);

    SamplePageSpec spec;
    spec.title = "ComboBox(组合框)";
    spec.subtitle = "组合框显示当前值，并打开列表进行更改。";
    spec.sections = {
        {
            "选择水果",
            "单击字段，或按 Alt+Down，然后选择一项。",
            Column(10).Children({
                combo,
                disabled,
                status,
            }).Build(),
        },
    };
    spec.source =
        "auto combo = std::make_shared<ComboBox>();\n"
        "combo->AddItem(\"Apple\");\n"
        "combo->OnSelectionChanged().Connect([](ComboBox*, int, const std::string& item) {\n"
        "    // use item\n"
        "});\n"
        "combo->SetSelectedIndex(0);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
