#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildListBoxPage() {
    SamplePageSpec spec;
    spec.title = "ListBox(列表框)";
    spec.subtitle = "显示可选择的项列表。";
    spec.sections = {
        {
            "常规用法",
            "示例页面构建中。",
            MakeStatus("内容待完善..."),
        },
    };
    spec.source = "// ListBox sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
