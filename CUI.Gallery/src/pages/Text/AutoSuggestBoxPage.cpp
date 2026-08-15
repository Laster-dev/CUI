#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildAutoSuggestBoxPage() {
    SamplePageSpec spec;
    spec.title = "AutoSuggestBox(自动建议框)";
    spec.subtitle = "输入时提供下拉搜索建议建议。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// AutoSuggestBox sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
