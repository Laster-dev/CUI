#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildDockPage(const ShowcaseContext&) {
    auto target = std::make_shared<DockPanel>();
    target->SetProperty("width", Value(420.0f));
    target->SetProperty("height", Value(240.0f));
    auto top = ElevatedButton("Top 顶部").Background("#007ACC").Height(36).Build(); top->SetProperty("DockPanel.Dock", Value("Top"));
    auto bottom = ElevatedButton("Bottom 底部").Background("#10B981").Height(32).Build(); bottom->SetProperty("DockPanel.Dock", Value("Bottom"));
    auto left = ElevatedButton("Left 左侧").Background("#8E44AD").Width(100).Build(); left->SetProperty("DockPanel.Dock", Value("Left"));
    auto center = ElevatedButton("Center 中央填充").Background("#D13438").Build();
    target->AddChild(top); target->AddChild(bottom); target->AddChild(left); target->AddChild(center);
    return { "DockPanel 侧边停靠", CreatePage(
        "DockPanel 侧边停靠控制台",
        "支持 Top/Bottom/Left/Right 侧边停靠与中央填充。",
        CreateDemoSurface({ target }, 0.0f),
        CreateRightPanel({
            CreateShowcaseText("停靠面板属性表 (DockPanel)", 12.0f, "#569CD6", true),
            CheckboxTile("末尾填充 (LastChildFill)").Build()
        })) };
}
