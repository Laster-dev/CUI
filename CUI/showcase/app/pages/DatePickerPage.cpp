#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildDatePickerPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref target = Widgets::DatePicker().Shared();
    return { "DatePicker 日期选择", CreatePage(
        "DatePicker 日期选择器控件",
        "支持年月日读取、快捷切换与标准格式化输出。",
        CreateDemoSurface({ target }, 0.0f)) };
}
