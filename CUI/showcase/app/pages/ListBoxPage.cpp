#include "framework/core/CUIDsl.h"
#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"

ShowcasePage BuildListBoxPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref target =std::make_shared<CUI::ListBox>();
        target.Width(340.0f);
        target.Height(280.0f);
        target.VirtualCount(100000);
        target.SelectedIndex(0);
    return { "ListBox (100k)", CreatePage(
        "ListBox 100k 虚拟化列表全属性控制台",
        "ListBox 可滚动列表。",
        CreateDemoSurface({ target }, 0.0f)) };
}
