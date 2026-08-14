#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildTitleBarPage() {
    SamplePageSpec spec;
    spec.title = "TitleBar(标题栏)";
    spec.subtitle = "自定义窗口标题栏及其内容布局。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// TitleBar sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
