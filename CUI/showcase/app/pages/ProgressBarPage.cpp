#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/ProgressBarDiag.h"

using namespace CUI::DSL;

ShowcasePage BuildProgressBarPage(const ShowcaseContext& ctx) {
    CUI::ProgressBarDiag::Log("[PB] BuildProgressBarPage enter");
    auto target = ProgressBarWidget(65.0f, false).Width(280).Height(3).Build();
    auto indeterminate = ProgressBarWidget(0.0f, true).Width(280).Height(3).Build();
    CUI::ProgressBarDiag::Log(
        "[PB] BuildProgressBarPage done target=%p indeterminate=%p indFlag=%d",
        (void*)target.get(),
        (void*)indeterminate.get(),
        indeterminate ? (indeterminate->IsIndeterminate() ? 1 : 0) : -1);
    return { "ProgressBar 进度条", CreatePage(
        "ProgressBar 进度条控件",
        "支持线性 ProgressBar 与环形 ProgressRing，含确定进度与 IsIndeterminate 加载动画。",
        CreateDemoSurface({
            CreateShowcaseText("1. 确定进度模式 (Value = 65%):", 12.0f, "#AAAAAA"),
            target,
            CreateShowcaseText("2. 不确定加载动画模式 (IsIndeterminate):", 12.0f, "#AAAAAA"),
            indeterminate,
            CreateShowcaseText("3. ProgressRing 环形加载 (IsIndeterminate):", 12.0f, "#AAAAAA"),
            ProgressRingWidget(0.0f, true).Width(40).Height(40).Build(),
            CreateShowcaseText("4. ProgressRing 确定进度 (Value = 65%):", 12.0f, "#AAAAAA"),
            ProgressRingWidget(65.0f, false).Width(40).Height(40).Build()
        }),
        CreatePropertyGrid(ctx, target), target) };
}
