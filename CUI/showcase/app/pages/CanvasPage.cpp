#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildCanvasPage(const ShowcaseContext&) {
    auto target = std::make_shared<Canvas>();
    CUI::DSL::Borrow(target).Width(460.0f);
    CUI::DSL::Borrow(target).Height(240.0f);
    auto button = ElevatedButton("绝对坐标定位元素").Background(Rgb(0x007ACC)).Padding(14, 8, 14, 8).Build();
    DSL::Borrow(button).CanvasLeft(40.0f);
    DSL::Borrow(button).CanvasTop(50.0f);
    CUI::DSL::Borrow(target).AddChild(button);
    return { "Canvas 画布", CreatePage(
        "Canvas 绝对定位画布控制台",
        "支持附加属性 Canvas.Left 与 Canvas.Top 自由坐标平移。",
        CreateDemoSurface({ target }, 0.0f)) };
}
