#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildExpanderPage() {
    SamplePageSpec spec;
    spec.title = "Expander(折叠控件)";
    spec.subtitle = "可通过点击展开或折叠内容的容器。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Expander sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
