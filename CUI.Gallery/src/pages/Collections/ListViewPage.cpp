#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildListViewPage() {
    SamplePageSpec spec;
    spec.title = "ListView(列表视图)";
    spec.subtitle = "显示数据项集合。";
    spec.sections = {
        {
            "常规用法",
            "示例页面构建中。",
            MakeStatus("内容待完善..."),
        },
    };
    spec.source = "// ListView sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
