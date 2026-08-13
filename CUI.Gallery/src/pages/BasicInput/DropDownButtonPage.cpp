#include "pages/BasicInput/Pages.h"
#include "page/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/DropDownButton.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildDropDownButtonPage() {
    auto file = std::make_shared<DropDownButton>("文件");
    auto status = MakeStatus("选择命令。");
    file->AddItem("新建");
    file->AddItem("打开");
    file->AddSeparator();
    file->AddItem("退出");
    file->OnItemChosen().Connect([status](DropDownButton*, int, const std::string& text) {
        status->SetText(text + "。");
    });

    auto disabled = std::make_shared<DropDownButton>("不可用");
    disabled->AddItem("一项");
    disabled->SetIsEnabled(false);

    SamplePageSpec spec;
    spec.title = "DropDownButton(下拉按钮)";
    spec.subtitle = "整个按钮打开菜单。适用于没有默认操作的场景。";
    spec.sections = {
        {
            "文件菜单",
            "单击按钮，或按空格 / Alt+Down，然后选择一项。",
            Column(10).Children({
                Row(12).Children({ file, disabled }).Build(),
                status,
            }).Build(),
        },
    };
    spec.source =
        "auto file = std::make_shared<DropDownButton>(\"File\");\n"
        "file->AddItem(\"New\", [] { /* ... */ });\n"
        "file->AddSeparator();\n"
        "file->AddItem(\"Exit\");\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
