#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/DropDownButton.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildDropDownButtonPage() {
    //经典展示
    auto file = std::make_shared<DropDownButton>("文件");
    auto status = MakeStatus("选择命令。");
    file->AddItem("新建");
    file->AddItem("打开");
    file->AddSeparator();
    file->AddItem("退出");
    file->OnItemChosen().Connect([status](DropDownButton*, int, const std::string& text) {
        status->SetText(text + "。");
    });
    //禁用展示
    auto disabled = std::make_shared<DropDownButton>("不可用");
    disabled->AddItem("一项");
    disabled->SetIsEnabled(false);
    //不同颜色的展示
    auto color = std::make_shared<DropDownButton>("背景颜色展示");
    color->AddItem("红色");
    color->AddItem("绿色");
    color->AddItem("蓝色");
    color->AddItem("黄色");
    color->AddItem("紫色");
    color->AddItem("青色");
    color->AddItem("灰色");
    color->AddItem("黑色");

    color->OnItemChosen().Connect([color, status](DropDownButton*, int, const std::string& text) {
        if (text == "红色") {
            color->SetBackgroundColor(Color::Red);
            
        } else if (text == "绿色") {
            color->SetBackgroundColor(Color::Green);//设置背景色
            color->SetHoverBackground(Color::Green);//设置悬停背景色
        } else if (text == "蓝色") {
            color->SetBackgroundColor(Color::Blue);
			color->SetHoverBackground(Color::Blue);
        } else if (text == "黄色") {
            color->SetBackgroundColor(Color::Yellow);
            color->SetHoverBackground(Color::Yellow);
        } else if (text == "紫色") {
            color->SetBackgroundColor(Color::Purple);
            color->SetHoverBackground(Color::Purple);
        } else if (text == "青色") {
            color->SetBackgroundColor(Color::Cyan);
            color->SetHoverBackground(Color::Cyan);
        } else if (text == "灰色") {
            color->SetBackgroundColor(Color::Gray);
            color->SetHoverBackground(Color::Gray);
        } else if (text == "黑色") {
            color->SetBackgroundColor(Color::Black);
            color->SetHoverBackground(Color::Black);
        } else {
            color->SetBackgroundColor(Color::White);
            color->SetHoverBackground(Color::White);
        }
        color->Blur();
        status->SetText("背景颜色已设置为" + text + "。");
    });


    SamplePageSpec spec;
    spec.title = "DropDownButton(下拉按钮)";
    spec.subtitle = "整个按钮打开菜单。适用于没有默认操作的场景。";
    spec.sections = {
        {
            "文件菜单",
            "单击按钮，或按空格 / Alt+Down，然后选择一项。",
            Column(10).Children({
                Row(12).Children({ file, disabled,color}).Build(),
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
