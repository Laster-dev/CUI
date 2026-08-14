#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildPieChartPage() {
    SamplePageSpec spec;
    spec.title = "PieChart(饼图)";
    spec.subtitle = "展示占比数据饼图。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// PieChart sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
