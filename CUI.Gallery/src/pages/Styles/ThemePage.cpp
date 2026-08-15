#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildThemePage() {
    SamplePageSpec spec;
    spec.title = "Theme(主题样式)";
    spec.subtitle = "展现应用在浅色与深色主题下的视觉规范。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Theme sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
