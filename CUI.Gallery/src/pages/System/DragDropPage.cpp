#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildDragDropPage() {
    SamplePageSpec spec;
    spec.title = "Drag and Drop(拖放)";
    spec.subtitle = "拖放操作与数据交换服务。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// DragDrop sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
