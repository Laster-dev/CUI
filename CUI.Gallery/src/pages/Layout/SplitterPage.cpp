#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildSplitterPage() {
    SamplePageSpec spec;
    spec.title = "Splitter(分隔条)";
    spec.subtitle = "允许用户拖拽调整相邻区域大小。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Splitter sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
