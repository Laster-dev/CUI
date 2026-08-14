#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildPopupRevealPage() {
    SamplePageSpec spec;
    spec.title = "Popup Reveal(浮层显现)";
    spec.subtitle = "Flyout 与菜单的弹出动画曲线。";
    spec.sections = { { "常规用法", "示例页面构建中。", MakeStatus("内容待完善...") } };
    spec.source = "// PopupReveal sample code\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
