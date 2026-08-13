#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"

ShowcasePage BuildBreadcrumbPage(const ShowcaseContext& ctx) {
    auto target = std::make_shared<CUI::BreadcrumbBar>();
    target->SetPath({ "CUI", "showcase", "Declarative Gallery" });
    return { "BreadcrumbBar 面包屑", CreatePage(
        "BreadcrumbBar 面包屑路径导航控件",
        "过长时前方折叠为 …，点击可选择被折叠的祖先节点；节点可直接点击跳转。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
