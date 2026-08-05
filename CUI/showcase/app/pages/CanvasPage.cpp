#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildCanvasPage(const ShowcaseContext&) {
    auto target = std::make_shared<Canvas>();
    target->SetWidth(460.0f);
    target->SetHeight(240.0f);
    auto button = ElevatedButton("绝对坐标定位元素").Background("#007ACC").Padding(14, 8, 14, 8).Build();
    button->SetCanvasLeft(40.0f);
    button->SetCanvasTop(50.0f);
    target->AddChild(button);
    return { "Canvas 画布", CreatePage(
        "Canvas 绝对定位画布控制台",
        "支持附加属性 Canvas.Left 与 Canvas.Top 自由坐标平移。",
        CreateDemoSurface({ target }, 0.0f),
        CreateRightPanel({
            CreateShowcaseText("画布附加属性表 (Canvas)", 12.0f, "#569CD6", true),
            CreateShowcaseText("横坐标 (Canvas.Left) [px]:", 11.0f, "#AAAAAA"),
            TextField("40").Width(280).Height(26).Build(),
            CreateShowcaseText("纵坐标 (Canvas.Top) [px]:", 11.0f, "#AAAAAA"),
            TextField("50").Width(280).Height(26).Build()
        })) };
}
