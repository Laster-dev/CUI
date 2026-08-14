#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildMenuBarPage() {
    SamplePageSpec spec;
    spec.title = "MenuBar(菜单栏)";
    spec.subtitle = "水平顶级应用菜单栏。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// MenuBar sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
