#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildPagingPage(const ShowcaseContext& ctx) {
    auto target = PagingControlWidget(1, 10).Build();
    return { "PagingControl 分页条", CreatePage(
        "PagingControl 数据表格/列表分页条控件",
        "支持页码快速切换、上一页/下一页导航与总页数约束。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
