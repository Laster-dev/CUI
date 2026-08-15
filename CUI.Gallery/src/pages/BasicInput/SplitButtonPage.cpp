#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/SplitButton.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildSplitButtonPage() {
    auto save = Make<SplitButton>("保存");
    save->Width = 120.0f;
    auto status = MakeStatus("单击主区域保存，或单击箭头选择其他格式。");
    
    save->OnClick().Connect([status](UIElement*) {
        status->Text = "已保存。";
    });
    
    save->AddItem("另存为 PDF", [status] { status->Text = "已保存为 PDF。"; });
    save->AddItem("另存为 PNG", [status] { status->Text = "已保存为 PNG。"; });
    save->AddSeparator();
    save->AddItem("保存副本", [status] { status->Text = "已保存副本。"; });

    auto color = Make<SplitButton>("红色");
    color->OnClick().Connect([status, color](UIElement*) {
        color->Background = Color::Red; status->Text = "已选择红色。"; color->Blur();
    });
    
    color->Background = Color::Red;
    color->HoverBackground = Color::Red;
    
    color->AddItem("红色", [status, color] { color->Background = Color::Red; color->HoverBackground = Color::Red; status->Text = "已选择红色。"; color->Blur(); });
    color->AddItem("绿色", [status, color] { color->Background = Color::Green; color->HoverBackground = Color::Green; status->Text = "已选择绿色。"; color->Blur(); });
    color->AddItem("蓝色", [status, color] { color->Background = Color::Blue; color->HoverBackground = Color::Blue; status->Text = "已选择蓝色。"; color->Blur(); });
    color->AddItem("黄色", [status, color] { color->Background = Color::Yellow; color->HoverBackground = Color::Yellow; status->Text = "已选择黄色。"; color->Blur(); });
    color->Width = 120.0f;

    SamplePageSpec spec;
    spec.title = "SplitButton(拆分按钮)";
    spec.subtitle = "主区域执行默认操作。箭头打开更多命令。";
    spec.sections = {
        {
            "保存及格式",
            "主按钮保存。箭头列出其他保存方式。",
            Column(10).Children({ save, color, status }).Build(),
        },
    };
    spec.source =
        "auto save = Make<SplitButton>(\"保存\");\n"
        "save->OnClick().Connect([](UIElement*) { /* default save */ });\n"
        "save->AddItem(\"另存为 PDF\");\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
