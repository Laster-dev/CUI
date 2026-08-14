#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildMarkdownViewPage() {
    SamplePageSpec spec;
    spec.title = "MarkdownView(Markdown 视图)";
    spec.subtitle = "渲染和展示 Markdown 格式文档。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// MarkdownView sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
