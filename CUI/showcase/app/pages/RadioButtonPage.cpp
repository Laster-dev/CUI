#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildRadioButtonPage(const ShowcaseContext& ctx) {
    auto radioA = Widgets::RadioButton("选项 1 (Option A)").GroupName("DemoGroup").Shared();
        radioA->SetState(CheckState::Checked);
    auto radioB = Widgets::RadioButton("选项 2 (Option B)").GroupName("DemoGroup").Shared();
    auto radioC = Widgets::RadioButton("选项 3 (Option C)").GroupName("DemoGroup").Shared();
    return { "RadioButton 单选框", CreatePage(
        "RadioButton 单选框控件",
        "支持 GroupName 互斥分组逻辑，同组内切换自动取消其他选项。",
        CreateDemoSurface({ radioA, radioB, radioC }, 12.0f)) };
}
