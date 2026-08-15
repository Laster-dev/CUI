#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildInfoBarPage() {
    SamplePageSpec spec;
    spec.title = "InfoBar(消息条)";
    spec.subtitle = "用于展示全应用或特定上下文的应用状态消息。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// InfoBar sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
