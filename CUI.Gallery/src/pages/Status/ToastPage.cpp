#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildToastPage() {
    SamplePageSpec spec;
    spec.title = "Toast(通知提示)";
    spec.subtitle = "应用内弹出的轻量即时通知。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Toast sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
