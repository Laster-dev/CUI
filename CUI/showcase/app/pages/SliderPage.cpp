#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildSliderPage(const ShowcaseContext& ctx) {
    auto target = Widgets::Slider().Width(280).Height(24).Shared();
    target->SetMinimum(0.0f);
    target->SetMaximum(100.0f);
    target->SetValue(45.0f);
    return { "Slider 滑块", CreatePage(
        "Slider 滑块控件",
        "支持水平与垂直方向、Step 步长约束、键盘上下左右微调与数值变动事件。",
        CreateDemoSurface({ target }, 0.0f)) };
}
