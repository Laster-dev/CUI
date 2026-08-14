#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildScrollViewerPage() {
    SamplePageSpec spec;
    spec.title = "ScrollViewer(滚动视图)";
    spec.subtitle = "包含超出屏幕边界内容的滚动容器。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// ScrollViewer sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
