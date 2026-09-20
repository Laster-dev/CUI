#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildTimePickerPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref target = Widgets::TimePicker().Shared();
    return { "TimePicker 时间选择", CreatePage(
        "TimePicker 时间选择器控件",
        "支持 HH:mm 格式化时间、快捷微调与时间变动回调。",
        CreateDemoSurface({ target }, 0.0f)) };
}
