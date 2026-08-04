#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"

ShowcasePage BuildBreadcrumbPage(const ShowcaseContext& ctx) {
    auto target = std::make_shared<CUI::BreadcrumbBar>();
    target->SetPath({ "CUI", "showcase", "Declarative Gallery" });
    return { "BreadcrumbBar 面包屑", CreatePage(
        "BreadcrumbBar 面包屑路径导航控件",
        "支持路径深层多节点、分隔符指示与节点直接点击跳转。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
