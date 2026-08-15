#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildDockManagerPage() {
    SamplePageSpec spec;
    spec.title = "DockManager(停靠管理器)";
    spec.subtitle = "高级 IDE 风格的面板停靠管理系统。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// DockManager sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
