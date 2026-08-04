#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildTimePickerPage(const ShowcaseContext& ctx) {
    auto target = TimePickerWidget().Build();
    return { "TimePicker 时间选择", CreatePage(
        "TimePicker 时间选择器控件",
        "支持 HH:mm 格式化时间、快捷微调与时间变动回调。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
