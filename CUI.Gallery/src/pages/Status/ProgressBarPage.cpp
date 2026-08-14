#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildProgressBarPage() {
    SamplePageSpec spec;
    spec.title = "ProgressBar(进度条)";
    spec.subtitle = "指示任务执行进度的水平线条。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// ProgressBar sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
