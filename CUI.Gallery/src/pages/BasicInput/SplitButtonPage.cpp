#include "pages/BasicInput/Pages.h"
#include "page/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/SplitButton.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildSplitButtonPage() {
    auto save = std::make_shared<SplitButton>("保存");
    auto status = MakeStatus("单击主区域保存，或单击箭头选择其他格式。");
    save->OnClick().Connect([status](UIElement*) {
        status->SetText("已保存。");
    });
    save->AddItem("另存为 PDF", [status] { status->SetText("已保存为 PDF。"); });
    save->AddItem("另存为 PNG", [status] { status->SetText("已保存为 PNG。"); });
    save->AddSeparator();
    save->AddItem("保存副本", [status] { status->SetText("已保存副本。"); });

    SamplePageSpec spec;
    spec.title = "SplitButton(拆分按钮)";
    spec.subtitle = "主区域执行默认操作。箭头打开更多命令。";
    spec.sections = {
        {
            "保存及格式",
            "主按钮保存。箭头列出其他保存方式。",
            Column(10).Children({ save, status }).Build(),
        },
    };
    spec.source =
        "auto save = std::make_shared<SplitButton>(\"Save\");\n"
        "save->OnClick().Connect([](UIElement*) { /* default save */ });\n"
        "save->AddItem(\"Save as PDF\");\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
