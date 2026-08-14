#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildTabViewPage() {
    SamplePageSpec spec;
    spec.title = "TabView(标签页视图)";
    spec.subtitle = "显示可切换的多标签选项卡。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// TabView sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
