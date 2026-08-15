#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildDockPanelPage() {
    SamplePageSpec spec;
    spec.title = "DockPanel(停靠面板)";
    spec.subtitle = "将子元素停靠在容器边缘的布局。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// DockPanel sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
