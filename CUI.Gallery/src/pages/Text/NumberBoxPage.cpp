#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildNumberBoxPage() {
    SamplePageSpec spec;
    spec.title = "NumberBox(数字输入框)";
    spec.subtitle = "支持增减按钮与数学表达式计算的数值输入框。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// NumberBox sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
