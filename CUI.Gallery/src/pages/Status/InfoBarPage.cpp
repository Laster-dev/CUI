#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

Element MakeStaticInfoBar(const std::string& title, const std::string& message, InfoBarSeverity severity) {
    auto bar = InfoBarWidget();
    CUI::DSL::Borrow(bar).Title(title);
    CUI::DSL::Borrow(bar).Message(message);
    CUI::DSL::Borrow(bar).Severity(severity);
    CUI::DSL::Borrow(bar).IsClosable(false);
    CUI::DSL::Borrow(bar).IsOpen(true);
    return bar;
}

} // namespace

Element BuildInfoBarPage() {
    // 可交互示例：可切换严重级别、可关闭、带操作按钮。
    auto demo = InfoBarWidget();
    CUI::DSL::Borrow(demo).Title("文件已保存");
    CUI::DSL::Borrow(demo).Message("你的更改已写入磁盘。撤销操作将在 30 秒后失效。");
    CUI::DSL::Borrow(demo).ActionText("撤销");
    CUI::DSL::Borrow(demo).IsClosable(true);
    CUI::DSL::Borrow(demo).IsOpen(true);

    auto actionStatus = MakeStatus("");
    demo->OnAction().Connect([actionStatus]() {
        actionStatus->Text = "已点击「撤销」操作按钮。";
    });
    demo->OnClosed().Connect([actionStatus]() {
        actionStatus->Text = "InfoBar 已关闭，可点击「显示」重新打开。";
    });

    auto severity = SegmentedWidget({ "信息", "成功", "警告", "错误" });
    CUI::DSL::Borrow(severity).SelectedIndex(0);
    severity->OnSelectionChanged().Connect([demo](SegmentedControl*, int index, const std::string&) {
        switch (index) {
        case 1: CUI::DSL::Borrow(demo).Severity(InfoBarSeverity::Success); break;
        case 2: CUI::DSL::Borrow(demo).Severity(InfoBarSeverity::Warning); break;
        case 3: CUI::DSL::Borrow(demo).Severity(InfoBarSeverity::Error); break;
        default: CUI::DSL::Borrow(demo).Severity(InfoBarSeverity::Informational); break;
        }
    });

    auto closable = ToggleSwitchTile("可关闭（右上角 X）", true);
    closable->OnToggled().Connect([demo](ToggleSwitch*, bool on) {
        CUI::DSL::Borrow(demo).IsClosable(on);
    });

    auto showBtn = ElevatedButton("显示", [demo](UIElement*) { CUI::DSL::Borrow(demo).IsOpen(true); });
    auto hideBtn = ElevatedButton("隐藏", [demo](UIElement*) { CUI::DSL::Borrow(demo).IsOpen(false); });

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
            ".Severity(InfoBarSeverity::.) 控制左侧图标、强调色与边框配色。",
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
        "infoBar.Title(\"文件已保存\");\n"
        "infoBar.Message(\"你的更改已写入磁盘。\");\n"
        "infoBar.Severity(InfoBarSeverity::Success);\n"
        "infoBar.ActionText(\"撤销\");\n"
        "infoBar.IsClosable(true);\n"
        "infoBar.IsOpen(true);\n"
        "infoBar->OnAction().Connect([]() { /* 处理操作 */ });\n"
        "infoBar->OnClosed().Connect([]() { /* 处理关闭 */ });\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery






