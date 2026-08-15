#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildBackdropPage() {
    SamplePageSpec spec;
    spec.title = "Backdrop(窗口背景)";
    spec.subtitle = "云母 (Mica) 与亚克力 (Acrylic) 窗口背景材质。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Backdrop sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
