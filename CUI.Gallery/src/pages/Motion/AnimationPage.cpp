#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildAnimationPage() {
    SamplePageSpec spec;
    spec.title = "Implicit Animations(隐式动画)";
    spec.subtitle = "悬停、展开、数值过渡演示。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Animation sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
