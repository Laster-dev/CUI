#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildLogViewPage() {
    SamplePageSpec spec;
    spec.title = "LogView(日志视图)";
    spec.subtitle = "展示结构化系统与运行日志。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// LogView sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
