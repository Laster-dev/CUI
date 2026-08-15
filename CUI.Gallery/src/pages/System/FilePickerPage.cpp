#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildFilePickerPage() {
    SamplePageSpec spec;
    spec.title = "FilePicker(文件选择器)";
    spec.subtitle = "选择单个或多个打开/保存文件。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// FilePicker sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
