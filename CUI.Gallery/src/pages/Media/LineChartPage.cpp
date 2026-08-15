#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildLineChartPage() {
    SamplePageSpec spec;
    spec.title = "LineChart(折线图)";
    spec.subtitle = "展示趋势与折线图表。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// LineChart sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
