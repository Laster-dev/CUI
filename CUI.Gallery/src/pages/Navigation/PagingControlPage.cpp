#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildPagingControlPage() {
    SamplePageSpec spec;
    spec.title = "PagingControl(分页控件)";
    spec.subtitle = "提供数据或页面的指示与切换。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// PagingControl sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
