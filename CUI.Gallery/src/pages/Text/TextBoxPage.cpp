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
    basic->SetPlaceholder("请输入内容");
    basic->SetWidth(300.0f);
    basic->SetHeight(28.0f);

    auto status = MakeStatus("在输入框中输入文字试试。");
    basic->OnTextChanged().Connect([status](TextBox*, const std::string& text) {
        const std::string shown = text.empty() ? "（空）" : text;
        status->SetText("当前文本（" + std::to_string(text.size()) + " 字符）：" + shown);
    });

    auto clear = Make<Button>("清空");
    clear->OnClick().Connect([basic, status](UIElement*) {
        basic->Text->Set("");
        status->SetText("已清空输入。");
    });

    auto multiline = Make<TextBox>();
    multiline->SetPlaceholder("多行文本：支持 Enter 换行与自动折行");
    multiline->SetWidth(340.0f);
    multiline->SetHeight(120.0f);
    multiline->SetAcceptsReturn(true);
    multiline->SetTextWrapping(true);

    auto readOnly = Make<TextBox>();
    readOnly->SetText("只读文本：SetIsReadOnly(true)");
    readOnly->SetWidth(300.0f);
    readOnly->SetHeight(28.0f);
    readOnly->SetIsReadOnly(true);

    auto passwordMode = Make<TextBox>();
    passwordMode->SetPlaceholder("密码模式（带明文切换眼睛）");
    passwordMode->SetWidth(300.0f);
    passwordMode->SetHeight(28.0f);
    passwordMode->SetIsPasswordMode(true);
    passwordMode->SetShowRevealButton(true);

    auto disabled = Make<TextBox>();
    disabled->SetPlaceholder("不可用");
    disabled->SetWidth(300.0f);
    disabled->SetHeight(28.0f);
    disabled->SetIsEnabled(false);

    State<std::string> boundText{ "绑定数据源：点击右侧按钮更新文本。" };
    auto bound = Make<TextBox>();
    bound->SetWidth(300.0f);
    bound->SetHeight(28.0f);
    bound->SetIsReadOnly(true);
    bound->Text->Bind(boundText, BindingMode::OneWay);

    auto update = Make<Button>("更新绑定");
    update->OnClick().Connect([boundText](UIElement*) {
        boundText = "已通过 State 更新：Text->Bind(State, OneWay)。";
    });

    auto drop = Make<TextBox>();
    drop->SetPlaceholder("支持拖放：拖入文本或文件路径");
    drop->SetWidth(300.0f);
    drop->SetHeight(28.0f);
    drop->SetAllowDrop(true);
    drop->SetToolTip("允许从外部拖入文本或文件路径");

    SamplePageSpec spec;
    spec.title = "TextBox(文本框)";
    spec.subtitle = "用于单行或多行文本输入的编辑框，支持占位符、只读、密码模式与拖放。";
    spec.sections = {
        {
            "单行输入",
            "聚焦输入，OnTextChanged 实时回调；通过 Text->Set 可程序化修改内容。",
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
            "只读框通过 Text->Bind(State, BindingMode::OneWay) 单向绑定数据源。",
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
        "box->SetPlaceholder(\"请输入内容\");\n"
        "box->SetAcceptsReturn(true);\n"
        "box->SetTextWrapping(true);\n"
        "box->SetIsReadOnly(true);\n"
        "box->OnTextChanged().Connect([](TextBox*, const std::string& text) { ... });\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
