#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildUniformGridPage() {
    SamplePageSpec spec;
    spec.title = "UniformGrid(等距网格)";
    spec.subtitle = "所有单元格大小完全一致的网格布局。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// UniformGrid sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
