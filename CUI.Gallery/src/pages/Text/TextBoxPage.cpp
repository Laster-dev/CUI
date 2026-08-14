#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildTextBoxPage() {
    SamplePageSpec spec;
    spec.title = "TextBox(文本框)";
    spec.subtitle = "用于单行或多行文本输入的编辑框。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// TextBox sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
