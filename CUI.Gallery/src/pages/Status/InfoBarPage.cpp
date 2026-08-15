#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

Element MakeStaticInfoBar(const std::string& title, const std::string& message, InfoBarSeverity severity) {
    auto bar = InfoBarWidget();
    bar->SetTitle(title);
    bar->SetMessage(message);
    bar->SetSeverity(severity);
    bar->SetIsClosable(false);
    bar->SetIsOpen(true);
    return bar;
}

} // namespace

Element BuildInfoBarPage() {
    // 可交互示例：可切换严重级别、可关闭、带操作按钮。
    auto demo = InfoBarWidget();
    demo->SetTitle("文件已保存");
    demo->SetMessage("你的更改已写入磁盘。撤销操作将在 30 秒后失效。");
    demo->SetActionText("撤销");
    demo->SetIsClosable(true);
    demo->SetIsOpen(true);

    auto actionStatus = MakeStatus("");
    demo->OnAction().Connect([actionStatus]() {
        actionStatus->Text = "已点击「撤销」操作按钮。";
    });
    demo->OnClosed().Connect([actionStatus]() {
        actionStatus->Text = "InfoBar 已关闭，可点击「显示」重新打开。";
    });

    auto severity = SegmentedWidget({ "信息", "成功", "警告", "错误" });
    severity->SetSelectedIndex(0);
    severity->OnSelectionChanged().Connect([demo](SegmentedControl*, int index, const std::string&) {
        switch (index) {
        case 1: demo->SetSeverity(InfoBarSeverity::Success); break;
        case 2: demo->SetSeverity(InfoBarSeverity::Warning); break;
        case 3: demo->SetSeverity(InfoBarSeverity::Error); break;
        default: demo->SetSeverity(InfoBarSeverity::Informational); break;
        }
    });

    auto closable = ToggleSwitchTile("可关闭（右上角 X）", true);
    closable->OnToggled().Connect([demo](ToggleSwitch*, bool on) {
        demo->SetIsClosable(on);
    });

    auto showBtn = ElevatedButton("显示", [demo](UIElement*) { demo->SetIsOpen(true); });
    auto hideBtn = ElevatedButton("隐藏", [demo](UIElement*) { demo->SetIsOpen(false); });

    SamplePageSpec spec;
    spec.title = "InfoBar(消息条)";
    spec.subtitle = "用于展示全应用或特定上下文的应用状态消息，支持四种严重级别与操作按钮。";
    spec.sections = {
        {
            "可交互示例",
            "切换严重级别、开关关闭按钮，点击操作按钮或关闭按钮观察反馈。",
            Column(12, {
                demo,
                Row(12, { severity }),
                Row(12, { closable, showBtn, hideBtn }),
                actionStatus,
            }),
        },
        {
            "四种严重级别",
            "SetSeverity(InfoBarSeverity::...) 控制左侧图标、强调色与边框配色。",
            Column(12, {
                MakeStaticInfoBar("信息", "这是一条普通提示，例如“检查更新中…”。", InfoBarSeverity::Informational),
                MakeStaticInfoBar("成功", "操作已成功完成，无需任何额外处理。", InfoBarSeverity::Success),
                MakeStaticInfoBar("警告", "磁盘空间不足，请及时清理临时文件。", InfoBarSeverity::Warning),
                MakeStaticInfoBar("错误", "无法连接到服务器，请检查网络后重试。", InfoBarSeverity::Error),
            }),
        },
    };
    spec.source =
        "auto infoBar = InfoBarWidget();\n"
        "infoBar->SetTitle(\"文件已保存\");\n"
        "infoBar->SetMessage(\"你的更改已写入磁盘。\");\n"
        "infoBar->SetSeverity(InfoBarSeverity::Success);\n"
        "infoBar->SetActionText(\"撤销\");\n"
        "infoBar->SetIsClosable(true);\n"
        "infoBar->SetIsOpen(true);\n"
        "infoBar->OnAction().Connect([]() { /* 处理操作 */ });\n"
        "infoBar->OnClosed().Connect([]() { /* 处理关闭 */ });\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
