#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildNumberBoxPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref target = Widgets::NumberBox().Width(140).Height(28).Shared();
    target.Value(12.5);
        target.Step(0.5f);
    return { "NumberBox 微调框", CreatePage(
        "NumberBox 数字微调框控件",
        "自绘输入框与 ▲/▼ 微调按钮；支持 Step、滚轮、上下键、范围限制。可输入表达式（如 1+2*3、(4+5)/2），失焦或回车后求值。",
        CreateDemoSurface({ target }, 0.0f)) };
}
