#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildScrollViewerPage(const ShowcaseContext&) {
    auto target = std::make_shared<ScrollViewer>();
    target->SetWidth(380.0f);
    target->SetHeight(240.0f);
    target->AddChild(Column(8).Padding(10).Children({
        ElevatedButton("可滚动列表项 #1").Background(Rgb(0x007ACC)).Padding(12, 6, 12, 6).Build(),
        ElevatedButton("可滚动列表项 #2").Background(Rgb(0x007ACC)).Padding(12, 6, 12, 6).Build(),
        ElevatedButton("可滚动列表项 #3").Background(Rgb(0x007ACC)).Padding(12, 6, 12, 6).Build(),
        ElevatedButton("可滚动列表项 #4").Background(Rgb(0x007ACC)).Padding(12, 6, 12, 6).Build(),
        ElevatedButton("可滚动列表项 #5").Background(Rgb(0x007ACC)).Padding(12, 6, 12, 6).Build(),
        ElevatedButton("可滚动列表项 #6").Background(Rgb(0x007ACC)).Padding(12, 6, 12, 6).Build(),
        ElevatedButton("可滚动列表项 #7").Background(Rgb(0x007ACC)).Padding(12, 6, 12, 6).Build(),
        ElevatedButton("可滚动列表项 #8").Background(Rgb(0x007ACC)).Padding(12, 6, 12, 6).Build()
    }).Build());
    return { "ScrollViewer 滚动容器", CreatePage(
        "ScrollViewer 滚动视口容器控制台",
        "支持溢出大尺寸子内容的平滑像素滚动与 Direct2D 滚动条渲染。",
        CreateDemoSurface({ target }, 0.0f)) };
}
