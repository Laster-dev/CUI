#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildBreadcrumbBarPage() {
    SamplePageSpec spec;
    spec.title = "BreadcrumbBar(面包屑导航)";
    spec.subtitle = "显示当前位置的文件或页面层级路径。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// BreadcrumbBar sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
