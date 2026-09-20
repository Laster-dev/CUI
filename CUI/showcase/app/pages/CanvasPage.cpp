#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildCanvasPage(const ShowcaseContext&) {
    CUI::Widgets::Ref target = CUI::Widgets::Canvas().Shared();
        target.Width(460.0f);
        target.Height(240.0f);
    CUI::Widgets::Ref button =ElevatedButton("绝对坐标定位元素").Background(Rgb(0x007ACC)).Padding(14, 8, 14, 8).Build();
        button.CanvasLeft(40.0f);
        button.CanvasTop(50.0f);
        target->AddChild(button);
    return { "Canvas 画布", CreatePage(
        "Canvas 绝对定位画布控制台",
        "支持附加属性 Canvas.Left 与 Canvas.Top 自由坐标平移。",
        CreateDemoSurface({ target }, 0.0f)) };
}
