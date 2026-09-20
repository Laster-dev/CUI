#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/InfoBar.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Toast.h"
#include "framework/input/Command.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildInfoBarPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref bar = CUI::Widgets::InfoBar().Shared();
        bar.Title("已连接到远程主机");
        bar.Message("会话已建立。可以继续同步文件，或在断开后从历史记录恢复。");
        bar.ActionText("查看详情");
        bar.Severity(InfoBarSeverity::Informational);
        bar.IsOpen(true);
        bar.IsClosable(true);

    CUI::Widgets::Ref log =std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("操作日志：就绪", 12.0f, "#B5CEA8", false, "Consolas"));

    CUI::Widgets::Ref details =std::make_shared<Command>([window = ctx.windowRef, log]() {
                log.Text("[InfoBar] Command 执行");
        if (window) {
            Toast::Show(window->GetRootElement().get(), "InfoBar", "操作按钮",
                        ToastType::Info, ToastCorner::BottomRight, 1600);
        }
    });
        details.Label("查看详情");
        bar.ActionCommand(details);

    bar->OnClosed().Connect([log]() {
                log.Text("[InfoBar] 已关闭");
    });

    auto show = [bar, log](InfoBarSeverity sev, const char* title, const char* msg, const char* action) {
        return [bar, log, sev, title, msg, action](UIElement*) {
                        bar.Severity(sev);
                        bar.Title(title);
                        bar.Message(msg);
                        bar.ActionText(action);
                        bar.IsOpen(true);
                        log.Text(std::string("[InfoBar] 打开 ") + title);
        };
    };

    CUI::Widgets::Ref btnInfo = CUI::Widgets::Button("信息").Shared();
    btnInfo->OnClick().Connect(show(
        InfoBarSeverity::Informational,
        "已连接到远程主机",
        "会话已建立。可以继续同步文件，或在断开后从历史记录恢复。",
        "查看详情"));
    CUI::Widgets::Ref btnOk = CUI::Widgets::Button("成功").Shared();
    btnOk->OnClick().Connect(show(
        InfoBarSeverity::Success,
        "项目已保存",
        "所有更改已写入磁盘。",
        "打开文件夹"));
    CUI::Widgets::Ref btnWarn = CUI::Widgets::Button("警告").Shared();
    btnWarn->OnClick().Connect(show(
        InfoBarSeverity::Warning,
        "磁盘空间不足",
        "剩余空间低于 2 GB，构建缓存可能会失败。",
        "清理"));
    CUI::Widgets::Ref btnErr = CUI::Widgets::Button("错误").Shared();
    btnErr->OnClick().Connect(show(
        InfoBarSeverity::Error,
        "无法推送到远端",
        "身份验证失败。请检查凭据后重试。",
        "重试"));

    CUI::Widgets::Ref btnClose = CUI::Widgets::Button("关闭").Shared();
    btnClose->OnClick().Connect([bar](UIElement*) {     bar.IsOpen(false); });
    CUI::Widgets::Ref btnOpen = CUI::Widgets::Button("打开").Shared();
    btnOpen->OnClick().Connect([bar](UIElement*) {     bar.IsOpen(true); });
    CUI::Widgets::Ref btnNoClose = CUI::Widgets::Button("禁止关闭").Shared();
    btnNoClose->OnClick().Connect([bar, log](UIElement*) {
                bar.IsClosable(!bar->GetIsClosable());
                log.Text(bar->GetIsClosable() ? "[InfoBar] 可关闭" : "[InfoBar] 不可关闭");
    });
    CUI::Widgets::Ref btnNoAction = CUI::Widgets::Button("无操作按钮").Shared();
    btnNoAction->OnClick().Connect([bar, log](UIElement*) {
        if (bar->GetActionText().empty()) {
                        bar.ActionText("查看详情");
                        log.Text("[InfoBar] 显示操作按钮");
        } else {
                        bar.ActionText("");
                        log.Text("[InfoBar] 隐藏操作按钮");
        }
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("页内横幅", 13.0f, "textPrimary", true),
            CreateShowcaseText("贴在内容顶部的持久状态。Toast 会消失；InfoBar 要用户处理或关掉。", 12.0f, "textSecondary", false),
            bar,
            Row(8).Children({ btnInfo, btnOk, btnWarn, btnErr }).Build(),
            Row(8).Children({ btnOpen, btnClose, btnNoClose, btnNoAction }).Build(),
            log,
        }, 10.0f),
    }).Build();

    return { "InfoBar 横幅", CreatePage(
        "InfoBar 信息栏",
        "信息 / 成功 / 警告 / 错误。自绘色条与图标；操作与关闭复用 Button；打开关闭有高度动画。",
        demo) };
}
