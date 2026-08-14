#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildCanvasPage() {
    SamplePageSpec spec;
    spec.title = "Canvas(画布)";
    spec.subtitle = "使用绝对坐标定位子元素的容器。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Canvas sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
