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
    auto basic = TextField()
        .Placeholder("请输入内容")
        .Width(300.0f)
        .Height(28.0f);

    State<std::string> textState{ "" };
    basic->Text.Bind(textState, BindingMode::TwoWay);
    auto statusValue = MakeComputed<std::string>([](const std::string& text) {
        const std::string shown = text.empty() ? "（空）" : text;
        return "当前文本（" + std::to_string(text.size()) + " 字符）：" + shown;
    }, textState);
    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto clear = Button("清空")
		.Width(80.0f)
        .OnClick([basic](UIElement*) {
            basic->Text.Set("");
            });

    auto multiline = TextField()
        .Placeholder("多行文本：支持 Enter 换行与自动折行")
        .Width(340.0f)
        .Height(120.0f);
    multiline->SetAcceptsReturn(true);
    multiline->SetTextWrapping(true);

    auto readOnly = TextField()
        .Text("只读文本：SetIsReadOnly(true)")
        .Width(300.0f)
        .Height(28.0f);
    readOnly->SetIsReadOnly(true);

    auto passwordMode = TextField()
        .Placeholder("密码模式（带明文切换眼睛）")
        .Width(300.0f)
        .Height(28.0f);
    passwordMode->SetIsPasswordMode(true);
    passwordMode->SetShowRevealButton(true);

    auto disabled = TextField()
        .Placeholder("不可用")
        .Width(300.0f)
        .Height(28.0f);
    disabled->IsEnabledProperty = false;

    State<std::string> boundText{ "绑定数据源：点击右侧按钮更新文本。" };
    auto bound = TextField()
        .Width(300.0f)
        .Height(28.0f);
    bound->SetIsReadOnly(true);
    bound->Text.Bind(boundText, BindingMode::OneWay);

    auto update = Button("更新绑定")
        .OnClick([boundText](UIElement*) {
            boundText = "已通过 State 更新：Text.Bind(State, OneWay)。";
            });

    auto drop = TextField()
        .Placeholder("支持拖放：拖入文本或文件路径")
        .Width(300.0f)
        .Height(28.0f);
    drop->SetAllowDrop(true);
    drop->ToolTip = "允许从外部拖入文本或文件路径";

    SamplePageSpec spec;
    spec.title = "TextBox(文本框)";
    spec.subtitle = "用于单行或多行文本输入的编辑框，支持占位符、只读、密码模式与拖放。";
    spec.sections = {
        {
            "单行输入",
            "Text 与 State 双向绑定，状态栏由 MakeComputed 派生；通过 Text.Set 可程序化修改内容。",
            Column(12, {
                basic,
                Row(8, {clear,status }),
            }),
        },
        {
            "多行输入",
            "SetAcceptsReturn(true) 允许回车换行，SetTextWrapping(true) 自动折行。",
            multiline,
        },
        {
            "状态与模式",
            "只读、密码模式（内置明文切换按钮）与不可用状态。",
            Column(12, {
                readOnly,
                passwordMode,
                disabled,
            }),
        },
        {
            "数据绑定",
            "只读框通过 Text.Bind(State, BindingMode::OneWay) 单向绑定数据源。",
            Row(8, { bound, update }),
        },
        {
            "拖放",
            "SetAllowDrop(true) 后可作为放置目标接收文本与文件路径。",
            drop,
        },
    };
    spec.source =
        "auto box = TextField();\n"
        "box->Placeholder = \"请输入内容\";\n"
        "box->SetAcceptsReturn(true);\n"
        "box->SetTextWrapping(true);\n"
        "box->SetIsReadOnly(true);\n"
        "State<std::string> text{ \"\" };\n"
        "box->Text.Bind(text, BindingMode::TwoWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
