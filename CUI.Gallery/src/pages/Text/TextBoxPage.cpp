#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/Button.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildTextBoxPage() {
    auto basic = Make<TextBox>();
    basic->Placeholder = "请输入内容";
    basic->Width = 300.0f;
    basic->Height = 28.0f;

    State<std::string> textState{ "" };
    basic->Text.Bind(textState, BindingMode::TwoWay);
    auto statusValue = MakeComputed<std::string>([](const std::string& text) {
        const std::string shown = text.empty() ? "（空）" : text;
        return "当前文本（" + std::to_string(text.size()) + " 字符）：" + shown;
    }, textState);
    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto clear = Make<Button>("清空");
    clear->OnClick().Connect([basic](UIElement*) {
        basic->Text.Set("");
    });

    auto multiline = Make<TextBox>();
    multiline->Placeholder = "多行文本：支持 Enter 换行与自动折行";
    multiline->Width = 340.0f;
    multiline->Height = 120.0f;
    multiline->SetAcceptsReturn(true);
    multiline->SetTextWrapping(true);

    auto readOnly = Make<TextBox>();
    readOnly->Text = "只读文本：SetIsReadOnly(true)";
    readOnly->Width = 300.0f;
    readOnly->Height = 28.0f;
    readOnly->SetIsReadOnly(true);

    auto passwordMode = Make<TextBox>();
    passwordMode->Placeholder = "密码模式（带明文切换眼睛）";
    passwordMode->Width = 300.0f;
    passwordMode->Height = 28.0f;
    passwordMode->SetIsPasswordMode(true);
    passwordMode->SetShowRevealButton(true);

    auto disabled = Make<TextBox>();
    disabled->Placeholder = "不可用";
    disabled->Width = 300.0f;
    disabled->Height = 28.0f;
    disabled->IsEnabledProperty = false;

    State<std::string> boundText{ "绑定数据源：点击右侧按钮更新文本。" };
    auto bound = Make<TextBox>();
    bound->Width = 300.0f;
    bound->Height = 28.0f;
    bound->SetIsReadOnly(true);
    bound->Text.Bind(boundText, BindingMode::OneWay);

    auto update = Make<Button>("更新绑定");
    update->OnClick().Connect([boundText](UIElement*) {
        boundText = "已通过 State 更新：Text.Bind(State, OneWay)。";
    });

    auto drop = Make<TextBox>();
    drop->Placeholder = "支持拖放：拖入文本或文件路径";
    drop->Width = 300.0f;
    drop->Height = 28.0f;
    drop->SetAllowDrop(true);
    drop->ToolTip = "允许从外部拖入文本或文件路径";

    SamplePageSpec spec;
    spec.title = "TextBox(文本框)";
    spec.subtitle = "用于单行或多行文本输入的编辑框，支持占位符、只读、密码模式与拖放。";
    spec.sections = {
        {
            "单行输入",
            "Text 与 State 双向绑定，状态栏由 MakeComputed 派生；通过 Text.Set 可程序化修改内容。",
            Column(12).Children({
                basic,
                Row(8).Children({ clear }).Build(),
                status,
            }).Build(),
        },
        {
            "多行输入",
            "SetAcceptsReturn(true) 允许回车换行，SetTextWrapping(true) 自动折行。",
            multiline,
        },
        {
            "状态与模式",
            "只读、密码模式（内置明文切换按钮）与不可用状态。",
            Column(12).Children({
                readOnly,
                passwordMode,
                disabled,
            }).Build(),
        },
        {
            "数据绑定",
            "只读框通过 Text.Bind(State, BindingMode::OneWay) 单向绑定数据源。",
            Row(8).Children({ bound, update }).Build(),
        },
        {
            "拖放",
            "SetAllowDrop(true) 后可作为放置目标接收文本与文件路径。",
            drop,
        },
    };
    spec.source =
        "auto box = Make<TextBox>();\n"
        "box->Placeholder = \"请输入内容\";\n"
        "box->SetAcceptsReturn(true);\n"
        "box->SetTextWrapping(true);\n"
        "box->SetIsReadOnly(true);\n"
        "State<std::string> text{ \"\" };\n"
        "box->Text.Bind(text, BindingMode::TwoWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
