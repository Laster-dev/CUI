#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildWrapPanelPage() {
    SamplePageSpec spec;
    spec.title = "WrapPanel(换行面板)";
    spec.subtitle = "按顺序排列子元素，超出边界时自动换行。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// WrapPanel sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
