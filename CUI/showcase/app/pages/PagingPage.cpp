#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildPagingPage(const ShowcaseContext& ctx) {
    auto target = PagingControlWidget(1, 10).Build();
    return { "PagingControl 分页条", CreatePage(
        "PagingControl 数据表格/列表分页条控件",
        "Fluent 风格自绘分页：Chevron 导航、滑动选中胶囊、省略号窗口与滚轮/键盘切换。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
