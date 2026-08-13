#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/TabView.h"
#include "framework/controls/TextBlock.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildTabViewPage(const ShowcaseContext& ctx) {
    auto tabView = std::make_shared<TabView>();
    tabView->SetWidth(520.0f);
    tabView->SetHeight(280.0f);

    auto page1 = Column(8).Padding(16).Children({
        std::make_shared<TextBlock>("Tab 1 — 首页"),
        std::make_shared<TextBlock>("TabView 支持多标签页切换、关闭与横向滚动。")
    }).Build();
    auto page2 = Column(8).Padding(16).Children({
        std::make_shared<TextBlock>("Tab 2 — 文档"),
        std::make_shared<TextBlock>("每个标签可绑定独立内容树。")
    }).Build();
    auto page3 = Column(8).Padding(16).Children({
        std::make_shared<TextBlock>("Tab 3 — 设置"),
        std::make_shared<TextBlock>("点击 × 可关闭标签（可配置）。")
    }).Build();

    tabView->AddTab("Home", page1, "", true);
    tabView->AddTab("Documents", page2, "", true);
    tabView->AddTab("Settings", page3, "", true);
    tabView->SetSelectedIndex(0);

    return { "TabView 标签页", CreatePage(
        "TabView 多标签页控件",
        "WinUI 风格标签栏：切换、关闭、溢出横向滚动与选中指示动画。",
        CreateDemoSurface({ tabView }, 0.0f)) };
}
