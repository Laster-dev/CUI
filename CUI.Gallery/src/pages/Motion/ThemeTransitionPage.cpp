#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildThemeTransitionPage() {
    SamplePageSpec spec;
    spec.title = "Theme Transition(主题过渡)";
    spec.subtitle = "带波纹扩展的主题切换过渡动画。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// ThemeTransition sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
