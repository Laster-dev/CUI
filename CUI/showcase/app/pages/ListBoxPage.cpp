#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"

ShowcasePage BuildListBoxPage(const ShowcaseContext&) {
    auto target = std::make_shared<CUI::ListBox>();
    target->SetProperty("width", CUI::Value(340.0f));
    target->SetProperty("height", CUI::Value(280.0f));
    target->SetVirtualCount(100000);
    target->SetSelectedIndex(0);
    return { "ListBox (100k)", CreatePage(
        "ListBox 100k 虚拟化列表全属性控制台",
        "0 内存分配的高性能 10 万项虚拟化列表，滚动平滑丝滑。",
        CreateDemoSurface({ target }, 0.0f),
        CreateRightScrollPanel({
            CreateShowcaseText("列表属性控制表 (ListBox)", 12.0f, "#569CD6", true),
            CreateShowcaseText("行高度 (ItemHeight) [px]:", 11.0f, "#AAAAAA"),
            CUI::DSL::TextField("28").Width(280).Height(26).Build(),
            CreateShowcaseText("显式宽度 (Width):", 11.0f, "#AAAAAA"),
            CUI::DSL::TextField("340").Width(280).Height(26).Build()
        })) };
}
