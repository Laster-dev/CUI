#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildTreeViewPage() {
    SamplePageSpec spec;
    spec.title = "TreeView(树形视图)";
    spec.subtitle = "显示具有层级关系的数据。";
    spec.sections = {
        {
            "常规用法",
            "示例页面构建中。",
            MakeStatus("内容待完善..."),
        },
    };
    spec.source = "// TreeView sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
