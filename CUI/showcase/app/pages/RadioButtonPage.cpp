#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildRadioButtonPage(const ShowcaseContext& ctx) {
    auto radioA = RadioButtonTile("选项 1 (Option A)", "DemoGroup").Build();
    radioA->SetState(CheckState::Checked);
    auto radioB = RadioButtonTile("选项 2 (Option B)", "DemoGroup").Build();
    auto radioC = RadioButtonTile("选项 3 (Option C)", "DemoGroup").Build();
    return { "RadioButton 单选框", CreatePage(
        "RadioButton 单选框控件",
        "支持 GroupName 互斥分组逻辑，同组内切换自动取消其他选项。",
        CreateDemoSurface({ radioA, radioB, radioC }, 12.0f),
        CreatePropertyGrid(ctx, radioA), radioA) };
}
