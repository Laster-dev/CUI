#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildStackPanelPage() {
    SamplePageSpec spec;
    spec.title = "StackPanel(堆栈面板)";
    spec.subtitle = "将子元素排成单行的水平或垂直序列。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// StackPanel sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
