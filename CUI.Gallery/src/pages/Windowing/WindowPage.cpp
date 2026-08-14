#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildWindowPage() {
    SamplePageSpec spec;
    spec.title = "Window(窗口)";
    spec.subtitle = "多窗口管理、透明度与窗口样式控制。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Window sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
