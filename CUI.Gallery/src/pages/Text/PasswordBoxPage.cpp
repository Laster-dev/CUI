#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildPasswordBoxPage() {
    SamplePageSpec spec;
    spec.title = "PasswordBox(密码框)";
    spec.subtitle = "用于隐藏输入字符的密码文本框。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// PasswordBox sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
