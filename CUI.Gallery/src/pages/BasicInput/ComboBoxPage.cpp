#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/ComboBox.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildComboBoxPage() {
    auto combo = ComboBoxWidget();
    combo->Width = 220.0f;
    combo->AddItem("苹果");
    combo->AddItem("香蕉");
    combo->AddItem("樱桃");
    combo->AddItem("橙子");

    State<int> selection{ 0 };
    combo->SelectedIndex.Bind(selection);

    auto statusValue = MakeComputed<std::string>([](int index) {
        switch (index) {
        case 0: return "已选择：苹果。";
        case 1: return "已选择：香蕉。";
        case 2: return "已选择：樱桃。";
        case 3: return "已选择：橙子。";
        default: return "请选择水果。";
        }
    }, selection);

    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto disabled = ComboBoxWidget();
    disabled->Width = 220.0f;
    disabled->AddItem("不可用");
    disabled->SetSelectedIndex(0);
    disabled->IsEnabledProperty = false;

    SamplePageSpec spec;
    spec.title = "ComboBox(组合框)";
    spec.subtitle = "组合框显示当前值，并打开列表进行更改。";
    spec.sections = {
        {
            "选择水果",
            "单击字段，或按 Alt+Down，然后选择一项。",
            Column(10, {
                combo,
                disabled,
                status,
            }),
        },
    };
    spec.source =
        "State<int> selection{ 0 };\n"
        "combo->SelectedIndex.Bind(selection);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
