#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/DropDownButton.h"

#include <memory>
#include <string>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {
namespace {

const char* FileCommandText(int index) {
    switch (index) {
    case 0: return "新建";
    case 1: return "打开";
    case 3: return "退出";
    default: return "";
    }
}

const char* ColorName(int index) {
    switch (index) {
    case 0: return "红色";
    case 1: return "绿色";
    case 2: return "蓝色";
    case 3: return "黄色";
    case 4: return "紫色";
    case 5: return "青色";
    case 6: return "灰色";
    case 7: return "黑色";
    default: return "";
    }
}

Color ColorForIndex(int index) {
    switch (index) {
    case 0: return Color::Red;
    case 1: return Color::Green;
    case 2: return Color::Blue;
    case 3: return Color::Yellow;
    case 4: return Color::Purple;
    case 5: return Color::Cyan;
    case 6: return Color::Gray;
    case 7: return Color::Black;
    default: return Color::White;
    }
}

Color TextColorForIndex(int index) {
    return index == 3 || index == 5 || index == 6 ? Color::Black : Color::White;
}

} // namespace

std::shared_ptr<UIElement> BuildDropDownButtonPage() {
    auto file = DropDownButtonWidget("文件");
    file->AddItem("新建");
    file->AddItem("打开");
    file->AddSeparator();
    file->AddItem("退出");

    State<int> fileSelection{ -1 };
    file->SelectedIndex->Bind(fileSelection);
    auto fileStatusValue = MakeComputed<std::string>([](int index) {
        const char* command = FileCommandText(index);
        return *command ? std::string("已选择命令：") + command + "。" : "选择命令。";
    }, fileSelection);
    auto fileStatus = MakeStatus("");
    fileStatus->Text->Bind(fileStatusValue, BindingMode::OneWay);

    auto disabled = DropDownButtonWidget("不可用");
    disabled->AddItem("一项");
    disabled->IsEnabledProperty = false;

    auto color = DropDownButtonWidget("背景颜色展示");
    color->AddItem("红色");
    color->AddItem("绿色");
    color->AddItem("蓝色");
    color->AddItem("黄色");
    color->AddItem("紫色");
    color->AddItem("青色");
    color->AddItem("灰色");
    color->AddItem("黑色");

    State<int> colorSelection{ -1 };
    color->SelectedIndex->Bind(colorSelection);
    auto backgroundColor = MakeComputed<Color>([](int index) {
        return ColorForIndex(index);
    }, colorSelection);
    auto textColor = MakeComputed<Color>([](int index) {
        return TextColorForIndex(index);
    }, colorSelection);
    auto colorStatusValue = MakeComputed<std::string>([](int index) {
        const char* name = ColorName(index);
        return *name ? std::string("背景颜色已设置为") + name + "。" : "选择背景颜色。";
    }, colorSelection);
    color->Background->Bind(backgroundColor, BindingMode::OneWay);
    color->HoverBackground->Bind(backgroundColor, BindingMode::OneWay);
    color->TextColor->Bind(textColor, BindingMode::OneWay);
    auto colorStatus = MakeStatus("");
    colorStatus->Text->Bind(colorStatusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "DropDownButton(下拉按钮)";
    spec.subtitle = "整个按钮打开菜单。选择状态和派生视觉均通过属性绑定同步。";
    spec.sections = {
        {
            "文件菜单",
            "单击按钮，或按空格 / Alt+Down，然后选择一项。",
            Column(10, {
                Row(12, {file, disabled, color }),
                fileStatus,
                colorStatus,
            }),
        },
    };
    spec.source =
        "State<int> colorSelection{ -1 };\n"
        "color->SelectedIndex->Bind(colorSelection);\n"
        "auto background = MakeComputed<Color>(ColorForIndex, colorSelection);\n"
        "color->Background->Bind(background, BindingMode::OneWay);\n"
        "color->HoverBackground->Bind(background, BindingMode::OneWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
