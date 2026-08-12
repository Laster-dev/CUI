#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildNumberBoxPage(const ShowcaseContext& ctx) {
    auto target = NumberBoxWidget(12.5).Width(140).Height(28).Build();
    target->SetStep(0.5f);
    return { "NumberBox 微调框", CreatePage(
        "NumberBox 数字微调框控件",
        "自绘输入框与 ▲/▼ 微调按钮，支持 Step 步长、滚轮、键盘上下键与范围限制。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
