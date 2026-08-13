#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildDockPage(const ShowcaseContext&) {
    auto target = std::make_shared<DockPanel>();
    target->SetWidth(420.0f);
    target->SetHeight(240.0f);
    auto top = ElevatedButton("Top 顶部").Background(Rgb(0x007ACC)).Height(36).Build(); top->SetDock(Dock::Top);
    auto bottom = ElevatedButton("Bottom 底部").Background(Rgb(0x10B981)).Height(32).Build(); bottom->SetDock(Dock::Bottom);
    auto left = ElevatedButton("Left 左侧").Background(Rgb(0x8E44AD)).Width(100).Build(); left->SetDock(Dock::Left);
    auto center = ElevatedButton("Center 中央填充").Background(Rgb(0xD13438)).Build();
    target->AddChild(top); target->AddChild(bottom); target->AddChild(left); target->AddChild(center);
    return { "DockPanel 侧边停靠", CreatePage(
        "DockPanel 侧边停靠控制台",
        "支持 Top/Bottom/Left/Right 侧边停靠与中央填充。",
        CreateDemoSurface({ target }, 0.0f)) };
}
