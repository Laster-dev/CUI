#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildBarChartPage() {
    SamplePageSpec spec;
    spec.title = "BarChart(柱状图)";
    spec.subtitle = "展示对比数据柱状图。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// BarChart sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
