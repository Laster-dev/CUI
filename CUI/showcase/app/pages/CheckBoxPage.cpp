#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildCheckBoxPage(const ShowcaseContext&) {
    auto target = CheckboxTile("交互功能开关").Build();
    target->SetState(CheckState::Checked);
    auto side = CreateRightScrollPanel({
        CreateShowcaseText("复选框属性控制表 (CheckBox)", 12.0f, "#569CD6", true),
        CreateShowcaseText("标签文本 (Text):", 11.0f, "#AAAAAA"),
        TextField("交互功能开关").Width(280).Height(26).Build(),
        CheckboxTile("是否启用 (IsEnabled)").Build(),
        CheckboxTile("三态模式 (IsThreeState)").Build(),
        CreateShowcaseText("选中状态 (CheckState):", 11.0f, "#AAAAAA"),
        []() {
            auto combo = std::make_shared<ComboBox>();
            combo->AddItem("Unchecked");
            combo->AddItem("Checked");
            combo->AddItem("Indeterminate");
            combo->SetSelectedIndex(1);
            combo->SetProperty("width", Value(280.0f));
            return std::static_pointer_cast<UIElement>(combo);
        }(),
        CreateShowcaseText("字体大小 (FontSize):", 11.0f, "#AAAAAA"),
        TextField("13").Width(280).Height(26).Build()
    });

    return { "CheckBox 复选框", CreatePage(
        "CheckBox 复选框全属性控制台",
        "支持选中(v)、未选中、以及三态不定状态(-)。",
        CreateDemoSurface({ target }, 0.0f),
        side) };
}
