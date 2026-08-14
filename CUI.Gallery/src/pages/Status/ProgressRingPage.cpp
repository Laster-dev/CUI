#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildProgressRingPage() {
    SamplePageSpec spec;
    spec.title = "ProgressRing(进度环)";
    spec.subtitle = "指示耗时任务的环形加载控件。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// ProgressRing sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
