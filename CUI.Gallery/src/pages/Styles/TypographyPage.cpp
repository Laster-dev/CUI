#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildTypographyPage() {
    SamplePageSpec spec;
    spec.title = "Typography(字体排印)";
    spec.subtitle = "字体、字号、字重层级展示。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Typography sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
