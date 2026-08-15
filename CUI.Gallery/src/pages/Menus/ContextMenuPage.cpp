#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildContextMenuPage() {
    SamplePageSpec spec;
    spec.title = "ContextMenu(上下文菜单)";
    spec.subtitle = "右键或长按触发的快捷菜单。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// ContextMenu sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
