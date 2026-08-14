#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildCommandsPage() {
    SamplePageSpec spec;
    spec.title = "Commands(命令与快捷键)";
    spec.subtitle = "应用命令与快捷键映射绑定。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Commands sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
