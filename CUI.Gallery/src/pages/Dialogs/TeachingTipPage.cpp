#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildTeachingTipPage() {
    SamplePageSpec spec;
    spec.title = "TeachingTip(气泡提示)";
    spec.subtitle = "用于向用户介绍新功能或进行引导。";
    spec.sections = {
        {
            "常规用法",
            "示例页面构建中。",
            MakeStatus("内容待完善..."),
        },
    };
    spec.source = "// TeachingTip sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
