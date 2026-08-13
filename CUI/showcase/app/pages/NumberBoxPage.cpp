#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildNumberBoxPage(const ShowcaseContext& ctx) {
    auto target = NumberBoxWidget(12.5).Width(140).Height(28).Build();
    target->SetStep(0.5f);
    return { "NumberBox 微调框", CreatePage(
        "NumberBox 数字微调框控件",
        "自绘输入框与 ▲/▼ 微调按钮；支持 Step、滚轮、上下键、范围限制。可输入表达式（如 1+2*3、(4+5)/2），失焦或回车后求值。",
        CreateDemoSurface({ target }, 0.0f)) };
}
