#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/core/Value.h"
#include "framework/controls/Expander.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildExpanderPage() {
    // —— 常规用法 ——
    auto formContent = Column(10, {
        MakeLabel("昵称", 12.0f, ThemeTokenId::TextSecondary, false),
        TextField("输入昵称").Width(280).Build(),
        Row(8, {ElevatedButton("保存").Background(Rgb(0x007ACC)).Padding(14, 8, 14, 8).Build(),
            ElevatedButton("取消").Padding(14, 8, 14, 8).Build(),
        }),
    });

    auto basicExpander = ExpanderWidget("账号设置");
    basicExpander->SetSubtitle("点击头部展开或折叠内容");
    basicExpander->SetContent(formContent);

    // —— 默认展开 + 状态事件 ——
    State<bool> expandedState{ true };

    auto termsContent = Column(8, {
        MakeLabel(
            "本示例演示 OnExpandedChanged 事件：切换展开状态时，下方状态栏会实时反馈。",
            12.0f, ThemeTokenId::TextMuted, false),
        MakeLabel(
            "第二行内容用于展示展开高度动画对多行内容的适应。",
            12.0f, ThemeTokenId::TextMuted, false),
    });

    auto eventExpander = ExpanderWidget("使用条款");
    eventExpander->SetSubtitle("默认处于展开状态");
    eventExpander->SetIsExpanded(true);
    eventExpander->SetContent(termsContent);
    eventExpander->OnExpandedChanged().Connect([expandedState](Expander*, bool expanded) {
        expandedState = expanded;
    });

    auto stateValue = MakeComputed<std::string>([](bool expanded) {
        return expanded ? "当前状态：已展开" : "当前状态：已折叠";
    }, expandedState);
    auto stateStatus = MakeStatus("");
    stateStatus->Text->Bind(stateValue, BindingMode::OneWay);

    // —— 向上展开 + 嵌套 ——
    auto nestedInner = ExpanderWidget("内层折叠面板");
    nestedInner->SetSubtitle("Expander 内可以继续嵌套 Expander");
    nestedInner->SetContent(Column(8, {
        MakeLabel("这是嵌套在最里层的内容。", 12.0f, ThemeTokenId::TextMuted, false),
    }));

    auto upExpander = ExpanderWidget("向上展开（嵌套示例）");
    upExpander->SetSubtitle("SetExpandDirection(Up) 后从底部向上展开");
    upExpander->SetExpandDirection(ExpandDirection::Up);
    upExpander->SetContent(nestedInner);

    SamplePageSpec spec;
    spec.title = "Expander(折叠控件)";
    spec.subtitle = "点击头部在展开与折叠之间切换，内容区带平滑的高度渐变动画。";
    spec.sections = {
        {
            "常规用法",
            "SetHeader / SetSubtitle 设置头部文本，SetContent 指定展开区内容，点击头部任意位置即可切换。",
            Column(12, {
                basicExpander,
                MakeStatus("默认处于折叠状态，点击“账号设置”头部展开表单。"),
            }),
        },
        {
            "默认展开与状态事件",
            "SetIsExpanded(true) 默认展开；OnExpandedChanged 在每次状态切换时触发。",
            Column(12, {
                eventExpander,
                stateStatus,
            }),
        },
        {
            "向上展开与嵌套",
            "SetExpandDirection(ExpandDirection::Up) 使内容从底部向上展开；内容区支持继续嵌套折叠面板。",
            Column(12, {
                upExpander,
                MakeStatus("外层向上展开，内层向下展开，两者动画互不干扰。"),
            }),
        },
    };
    spec.source =
        "auto expander = ExpanderWidget(\"账号设置\");\n"
        "expander->SetSubtitle(\"点击头部展开或折叠内容\");\n"
        "expander->SetContent(formContent);\n"
        "\n"
        "// 默认展开 + 事件监听\n"
        "expander->SetIsExpanded(true);\n"
        "expander->OnExpandedChanged().Connect([](Expander*, bool expanded) {\n"
        "    status->Text = expanded ? \"已展开\" : \"已折叠\";\n"
        "});\n"
        "\n"
        "// 向上展开\n"
        "expander->SetExpandDirection(ExpandDirection::Up);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
