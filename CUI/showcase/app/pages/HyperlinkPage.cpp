#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildHyperlinkPage(const ShowcaseContext& ctx) {
    auto target = HyperlinkButtonWidget("访问 GitHub 官方主页", "https://github.com").Build();
    return { "Hyperlink 超链接", CreatePage(
        "HyperlinkButton 超链接按钮全属性控制台",
        "由 PropertyGrid 反射绑定；颜色仅通过 theme.*Token 走 ThemeManager。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target)) };
}
