#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"

ShowcasePage BuildListBoxPage(const ShowcaseContext& ctx) {
    auto target = std::make_shared<CUI::ListBox>();
    target->SetWidth(340.0f);
    target->SetHeight(280.0f);
    target->SetVirtualCount(100000);
    target->SetSelectedIndex(0);
    return { "ListBox (100k)", CreatePage(
        "ListBox 100k 虚拟化列表全属性控制台",
        "由 PropertyGrid 反射绑定；颜色仅通过 theme.*Token 走 ThemeManager。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
