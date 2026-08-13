#include "pages/BasicInput/Pages.h"
#include "page/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/ToggleButton.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildToggleButtonPage() {
    auto bold = std::make_shared<ToggleButton>("粗体");
    auto italic = std::make_shared<ToggleButton>("斜体");
    auto status = MakeStatus("常规。");
    auto update = [bold, italic, status]() {
        const bool b = bold->IsChecked();
        const bool i = italic->IsChecked();
        if (b && i) {
            status->SetText("粗斜体。");
        } else if (b) {
            status->SetText("粗体。");
        } else if (i) {
            status->SetText("斜体。");
        } else {
            status->SetText("常规。");
        }
    };
    bold->OnToggled().Connect([update](ToggleButton*, bool) { update(); });
    italic->OnToggled().Connect([update](ToggleButton*, bool) { update(); });

    auto locked = std::make_shared<ToggleButton>("已锁定");
    locked->SetIsChecked(true);
    locked->SetIsEnabled(false);

    SamplePageSpec spec;
    spec.title = "ToggleButton(切换按钮)";
    spec.subtitle = "保持开或关的按钮。适用于粗体等模式，或工具栏按下状态。";
    spec.sections = {
        {
            "文本样式",
            "单击以锁定样式。多个切换可同时打开。",
            Column(10).Children({
                Row(12).Children({ bold, italic, locked }).Build(),
                status,
            }).Build(),
        },
    };
    spec.source =
        "auto bold = std::make_shared<ToggleButton>(\"Bold\");\n"
        "bold->OnToggled().Connect([](ToggleButton*, bool on) {\n"
        "    // apply style\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
