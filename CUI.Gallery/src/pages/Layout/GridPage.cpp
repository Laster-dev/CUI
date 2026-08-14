#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildGridPage() {
    SamplePageSpec spec;
    spec.title = "Grid(网格)";
    spec.subtitle = "按行和列组织元素的布局容器。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Grid sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
