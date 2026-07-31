#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildProgressBarPage(const ShowcaseContext& ctx) {
    auto target = ProgressBarWidget(65.0f, false).Width(280).Height(8).Build();
    auto indeterminate = ProgressBarWidget(0.0f, true).Width(280).Height(8).Build();
    return { "ProgressBar 进度条", CreatePage(
        "ProgressBar 进度条控件",
        "支持确定进度 Value 绘制与 isIndeterminate=true 动画不确定模式。",
        CreateDemoSurface({
            CreateShowcaseText("1. 确定进度模式 (Value = 65%):", 12.0f, "#AAAAAA"),
            target,
            CreateShowcaseText("2. 不确定加载动画模式 (IsIndeterminate):", 12.0f, "#AAAAAA"),
            indeterminate
        }),
        CreatePropertyGrid(ctx, target)) };
}
