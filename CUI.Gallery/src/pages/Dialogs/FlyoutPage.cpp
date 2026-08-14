#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildFlyoutPage() {
    SamplePageSpec spec;
    spec.title = "Flyout(浮出层)";
    spec.subtitle = "显示上下文提示或轻量交互面板。";
    spec.sections = {
        {
            "常规用法",
            "示例页面构建中。",
            MakeStatus("内容待完善..."),
        },
    };
    spec.source = "// Flyout sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
