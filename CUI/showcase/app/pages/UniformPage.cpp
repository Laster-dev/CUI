#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildUniformPage(const ShowcaseContext&) {
    auto target = Widgets::UniformGrid(2, 3).Width(420).Height(240).Shared();
    for (int i = 1; i <= 6; ++i) {
                target->AddChild(ElevatedButton("单元格 #" + std::to_string(i)).Background(Rgb(0x007ACC)).Build());
    }
    return { "UniformGrid 等分网格", CreatePage(
        "UniformGrid 等分网格控制台",
        "等比例切割网格，所有子元素自动平分空间。",
        CreateDemoSurface({ target }, 0.0f)) };
}
