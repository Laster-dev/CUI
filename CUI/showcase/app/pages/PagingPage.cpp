#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildPagingPage(const ShowcaseContext& ctx) {
    auto target = Widgets::PagingControl().Shared();
    target->SetCurrentPage(1);
    target->SetTotalPages(10);
    return { "PagingControl 分页条", CreatePage(
        "PagingControl 数据表格/列表分页条控件",
        "Fluent 风格自绘分页：Chevron 导航、滑动选中胶囊、省略号窗口与滚轮/键盘切换。",
        CreateDemoSurface({ target }, 0.0f)) };
}
