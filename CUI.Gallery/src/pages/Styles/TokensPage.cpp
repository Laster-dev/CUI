#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildTokensPage() {
    SamplePageSpec spec;
    spec.title = "Color Tokens(颜色令牌)";
    spec.subtitle = "展示 CUI ThemeTokenId 颜色规范矩阵。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Tokens sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
