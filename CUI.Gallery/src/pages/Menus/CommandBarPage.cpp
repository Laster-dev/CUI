#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildCommandBarPage() {
    SamplePageSpec spec;
    spec.title = "CommandBar(命令栏)";
    spec.subtitle = "提供轻量级命令与工具按钮组合。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// CommandBar sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
