#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildToolTipPage() {
    SamplePageSpec spec;
    spec.title = "ToolTip(工具提示)";
    spec.subtitle = "当鼠标悬停在控件上时显示的简单说明。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// ToolTip sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
