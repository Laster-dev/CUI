#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildFolderPickerPage() {
    SamplePageSpec spec;
    spec.title = "FolderPicker(文件夹选择器)";
    spec.subtitle = "选择目标文件夹目录。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// FolderPicker sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
