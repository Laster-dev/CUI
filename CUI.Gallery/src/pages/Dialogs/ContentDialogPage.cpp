#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildContentDialogPage() {
    SamplePageSpec spec;
    spec.title = "ContentDialog(内容对话框)";
    spec.subtitle = "显示模式对话框以确认或收集信息。";
    spec.sections = {
        {
            "常规用法",
            "示例页面构建中。",
            MakeStatus("内容待完善..."),
        },
    };
    spec.source = "// ContentDialog sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
