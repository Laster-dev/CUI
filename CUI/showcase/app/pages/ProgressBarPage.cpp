#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 CUI::Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/ProgressBarDiag.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildProgressBarPage(const ShowcaseContext& ctx) {
    CUI::ProgressBarDiag::Log("[PB] BuildProgressBarPage enter");
    CUI::Widgets::Ref target = CUI::Widgets::ProgressBar().Width(280).Height(3).Shared();
    target.Value(65.0f);
    target.IsIndeterminate(false);
    CUI::Widgets::Ref indeterminate = CUI::Widgets::ProgressBar().Width(280).Height(3).Shared();
    indeterminate.Value(0.0f);
    indeterminate.IsIndeterminate(true);
    CUI::Widgets::Ref ringIndeterminate = CUI::Widgets::ProgressRing().Width(40).Height(40).Shared();
    ringIndeterminate.Value(0.0f);
    ringIndeterminate.IsIndeterminate(true);
    CUI::Widgets::Ref ringDeterminate = CUI::Widgets::ProgressRing().Width(40).Height(40).Shared();
    ringDeterminate.Value(65.0f);
    ringDeterminate.IsIndeterminate(false);
    CUI::ProgressBarDiag::Log(
        "[PB] BuildProgressBarPage done target=%p indeterminate=%p indFlag=%d",
        (void*)target.get(),
        (void*)indeterminate.get(),
        indeterminate ? (indeterminate->IsIndeterminate() ? 1 : 0) : -1);
    return { "ProgressBar 进度条", CreatePage(
        "ProgressBar 进度条控件",
        "支持线性 ProgressBar 与环形 ProgressRing。ProgressRing 不确定模式为 WinUI 风格伸缩+旋转（非匀速扫弧）。",
        CreateDemoSurface({
            CreateShowcaseText("1. 确定进度模式 (Value = 65%):", 12.0f, "#AAAAAA"),
            target,
            CreateShowcaseText("2. 不确定加载动画模式 (IsIndeterminate):", 12.0f, "#AAAAAA"),
            indeterminate,
            CreateShowcaseText("3. ProgressRing WinUI 风格环形加载 (IsIndeterminate):", 12.0f, "#AAAAAA"),
            ringIndeterminate,
            CreateShowcaseText("4. ProgressRing 确定进度 (Value = 65%):", 12.0f, "#AAAAAA"),
            ringDeterminate
        })) };
}
