#include "Gallery.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildTextBoxPage() {
    auto basic = Widgets::TextBox()
        .Placeholder("请输入内容")
        .Width(300.0f)
        .Height(32.0f)
        .Shared();

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
        .Height(32.0f)
        .OnClick([basic](UIElement*) {
            basic->Text.Set("");
        });

    auto multiline = Widgets::TextBox()
        .Placeholder("多行文本：支持 Enter 换行与自动折行")
        .Width(340.0f)
        .Height(120.0f)
        .Shared();
        multiline->SetAcceptsReturn(true);
        multiline->SetTextWrapping(true);

    auto readOnly = Widgets::TextBox()
        .Text("只读文本：.IsReadOnly(true)")
        .Width(300.0f)
        .Height(32.0f)
        .Shared();
        readOnly->SetIsReadOnly(true);

    auto passwordMode = Widgets::TextBox()
        .Placeholder("密码模式（带明文切换眼睛）")
        .Width(300.0f)
        .Height(32.0f)
        .Shared();
        passwordMode->SetIsPasswordMode(true);
        passwordMode->SetShowRevealButton(true);

    auto disabled = Widgets::TextBox()
        .Placeholder("不可用")
        .Width(300.0f)
        .Height(32.0f)
        .Shared();
        disabled->SetIsEnabled(false);

    State<std::string> boundText{ "绑定数据源：点击右侧按钮更新文本。" };
    auto bound = Widgets::TextBox()
        .Width(300.0f)
        .Height(32.0f)
        .Shared();
        bound->SetIsReadOnly(true);
    bound->Text.Bind(boundText, BindingMode::OneWay);

    auto update = Button("更新绑定")
        .Height(32.0f)
        .AlignHorizontal(Alignment::Center)
        .OnClick([boundText](UIElement*) {
            boundText = "已通过 State 更新：Text.Bind(State, OneWay)。";
        });

    auto drop = Widgets::TextBox()
        .Placeholder("支持拖放：拖入文本或文件路径")
        .Width(300.0f)
        .Height(32.0f)
        .Shared();
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
                Row(8, { clear, status }),
            }),
        },
        {
            "多行输入",
            ".AcceptsReturn(true) 允许回车换行，.TextWrapping(true) 自动折行。",
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
            ".AllowDrop(true) 后可作为放置目标接收文本与文件路径。",
            drop,
        },
    };
    spec.source =
        "auto box = Widgets::TextBox().Shared();\n"
        "box->Placeholder = \"请输入内容\";\n"
        "box.AcceptsReturn(true);\n"
        "box.TextWrapping(true);\n"
        "box.IsReadOnly(true);\n"
        "State<std::string> text{ \"\" };\n"
        "box->Text.Bind(text, BindingMode::TwoWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery

