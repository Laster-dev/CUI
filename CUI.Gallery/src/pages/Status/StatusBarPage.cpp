#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildStatusBarPage() {
    SamplePageSpec spec;
    spec.title = "StatusBar(状态栏)";
    spec.subtitle = "底部状态栏指示。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// StatusBar sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
