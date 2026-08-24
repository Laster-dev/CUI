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
    auto bar = std::make_shared<InfoBar>();
    DSL::Borrow(bar).Title("已连接到远程主机");
    DSL::Borrow(bar).Message("会话已建立。可以继续同步文件，或在断开后从历史记录恢复。");
    DSL::Borrow(bar).ActionText("查看详情");
    DSL::Borrow(bar).Severity(InfoBarSeverity::Informational);
    CUI::DSL::Borrow(bar).IsOpen(true);
    DSL::Borrow(bar).IsClosable(true);

    auto log = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("操作日志：就绪", 12.0f, "#B5CEA8", false, "Consolas"));

    auto details = std::make_shared<Command>([window = ctx.windowRef, log]() {
        CUI::DSL::Borrow(log).Text("[InfoBar] Command 执行");
        if (window) {
            Toast::Show(window->GetRootElement().get(), "InfoBar", "操作按钮",
                        ToastType::Info, ToastCorner::BottomRight, 1600);
        }
    });
    DSL::Borrow(details).Label("查看详情");
    DSL::Borrow(bar).ActionCommand(details);

    bar->OnClosed().Connect([log]() {
        CUI::DSL::Borrow(log).Text("[InfoBar] 已关闭");
    });

    auto show = [bar, log](InfoBarSeverity sev, const char* title, const char* msg, const char* action) {
        return [bar, log, sev, title, msg, action](UIElement*) {
            DSL::Borrow(bar).Severity(sev);
            DSL::Borrow(bar).Title(title);
            DSL::Borrow(bar).Message(msg);
            DSL::Borrow(bar).ActionText(action);
            CUI::DSL::Borrow(bar).IsOpen(true);
            CUI::DSL::Borrow(log).Text(std::string("[InfoBar] 打开 ") + title);
        };
    };

    auto btnInfo = std::make_shared<Button>("信息");
    btnInfo->OnClick().Connect(show(
        InfoBarSeverity::Informational,
        "已连接到远程主机",
        "会话已建立。可以继续同步文件，或在断开后从历史记录恢复。",
        "查看详情"));
    auto btnOk = std::make_shared<Button>("成功");
    btnOk->OnClick().Connect(show(
        InfoBarSeverity::Success,
        "项目已保存",
        "所有更改已写入磁盘。",
        "打开文件夹"));
    auto btnWarn = std::make_shared<Button>("警告");
    btnWarn->OnClick().Connect(show(
        InfoBarSeverity::Warning,
        "磁盘空间不足",
        "剩余空间低于 2 GB，构建缓存可能会失败。",
        "清理"));
    auto btnErr = std::make_shared<Button>("错误");
    btnErr->OnClick().Connect(show(
        InfoBarSeverity::Error,
        "无法推送到远端",
        "身份验证失败。请检查凭据后重试。",
        "重试"));

    auto btnClose = std::make_shared<Button>("关闭");
    btnClose->OnClick().Connect([bar](UIElement*) { DSL::Borrow(bar).IsOpen(false); });
    auto btnOpen = std::make_shared<Button>("打开");
    btnOpen->OnClick().Connect([bar](UIElement*) { DSL::Borrow(bar).IsOpen(true); });
    auto btnNoClose = std::make_shared<Button>("禁止关闭");
    btnNoClose->OnClick().Connect([bar, log](UIElement*) {
        DSL::Borrow(bar).IsClosable(!bar->GetIsClosable());
        CUI::DSL::Borrow(log).Text(bar->GetIsClosable() ? "[InfoBar] 可关闭" : "[InfoBar] 不可关闭");
    });
    auto btnNoAction = std::make_shared<Button>("无操作按钮");
    btnNoAction->OnClick().Connect([bar, log](UIElement*) {
        if (bar->GetActionText().empty()) {
            DSL::Borrow(bar).ActionText("查看详情");
            CUI::DSL::Borrow(log).Text("[InfoBar] 显示操作按钮");
        } else {
            DSL::Borrow(bar).ActionText("");
            CUI::DSL::Borrow(log).Text("[InfoBar] 隐藏操作按钮");
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
