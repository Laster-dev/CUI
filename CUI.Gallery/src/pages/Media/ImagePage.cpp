#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildImagePage() {
    SamplePageSpec spec;
    spec.title = "Image(图像)";
    spec.subtitle = "展示静态图像或图像流。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// Image sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
