#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildDatePickerPage(const ShowcaseContext& ctx) {
    auto target = DatePickerWidget().Build();
    return { "DatePicker 日期选择", CreatePage(
        "DatePicker 日期选择器控件",
        "支持年月日读取、快捷切换与标准格式化输出。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
