#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildTextBlockPage() {
    SamplePageSpec spec;
    spec.title = "TextBlock(文本块)";
    spec.subtitle = "用于显示少量或多行文本。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// TextBlock sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
