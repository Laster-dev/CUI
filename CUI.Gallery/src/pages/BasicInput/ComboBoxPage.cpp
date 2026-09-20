#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildComboBoxPage() {
    CUI::Widgets::Ref combo = Widgets::ComboBox()
        .Width(220.0f).Shared();
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

    CUI::Widgets::Ref disabled = Widgets::ComboBox()
        .Width(220.0f).Shared();
    disabled->AddItem("不可用");
        disabled.SelectedIndex(0);
        disabled.IsEnabled(false);

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



