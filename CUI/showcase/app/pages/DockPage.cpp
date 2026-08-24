#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildDockPage(const ShowcaseContext&) {
    auto target = std::make_shared<DockPanel>();
    CUI::DSL::Borrow(target).Width(420.0f);
    CUI::DSL::Borrow(target).Height(240.0f);
    auto top = ElevatedButton("Top 顶部").Background(Rgb(0x007ACC)).Height(36).Build(); CUI::DSL::Borrow(top).Dock(Dock::Top);
    auto bottom = ElevatedButton("Bottom 底部").Background(Rgb(0x10B981)).Height(32).Build(); CUI::DSL::Borrow(bottom).Dock(Dock::Bottom);
    auto left = ElevatedButton("Left 左侧").Background(Rgb(0x8E44AD)).Width(100).Build(); CUI::DSL::Borrow(left).Dock(Dock::Left);
    auto center = ElevatedButton("Center 中央填充").Background(Rgb(0xD13438)).Build();
    CUI::DSL::Borrow(target).AddChild(top); target->AddChild(bottom); target->AddChild(left); target->AddChild(center);
    return { "DockPanel 侧边停靠", CreatePage(
        "DockPanel 侧边停靠控制台",
        "支持 Top/Bottom/Left/Right 侧边停靠与中央填充。",
        CreateDemoSurface({ target }, 0.0f)) };
}

