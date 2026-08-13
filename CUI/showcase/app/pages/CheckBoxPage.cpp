#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildCheckBoxPage(const ShowcaseContext& ctx) {
    auto target = CheckboxTile("交互功能开关").Build();
    target->SetState(CheckState::Checked);

    return { "CheckBox 复选框", CreatePage(
        "CheckBox 复选框全属性控制台",
        "CheckBox 三态与点击切换。",
        CreateDemoSurface({ target }, 0.0f)) };
}
