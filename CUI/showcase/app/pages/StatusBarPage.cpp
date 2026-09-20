#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/StatusBar.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildStatusBarPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref bar = CUI::Widgets::StatusBar().Shared();
        bar.Width(-1.0f);
        bar.Height(26.0f);

    const int msgId = bar->AddTextItem("就绪", StatusBarItemAlignment::Left);
        bar.ItemIcon(msgId, "●");
    bar->AddTextItem("CUI Control Gallery", StatusBarItemAlignment::Fill);
    bar->AddSeparator(StatusBarItemAlignment::Right);
    const int dpiId = bar->AddTextItem("DPI 100%", StatusBarItemAlignment::Right);
    bar->AddSeparator(StatusBarItemAlignment::Right);
    const int zoomId = bar->AddTextItem("缩放 100%", StatusBarItemAlignment::Right);
    bar->AddSeparator(StatusBarItemAlignment::Right);
    const int progId = bar->AddProgressItem("索引", StatusBarItemAlignment::Right, 140.0f);
        bar.ItemProgress(progId, 0.35f);

    auto hint = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("底栏：左消息 + 中间弹性标题 + 右侧 DPI / 缩放 / 进度（纯自绘，无 TextBlock 子控件）。", 12.0f, "textSecondary", false));

    CUI::Widgets::Ref btnBusy = CUI::Widgets::Button("模拟忙碌").Shared();
    btnBusy->OnClick().Connect([bar, msgId, progId](UIElement*) {
                bar.ItemText(msgId, "正在处理…");
                bar.ItemProgress(progId, 0.72f);
    });
    CUI::Widgets::Ref btnReady = CUI::Widgets::Button("恢复就绪").Shared();
    btnReady->OnClick().Connect([bar, msgId, progId, dpiId, zoomId](UIElement*) {
                bar.ItemText(msgId, "就绪");
                bar.ItemProgress(progId, 0.35f);
                bar.ItemText(dpiId, "DPI 100%");
                bar.ItemText(zoomId, "缩放 100%");
    });
    CUI::Widgets::Ref btnDpi = CUI::Widgets::Button("切换 DPI 文案").Shared();
    btnDpi->OnClick().Connect([bar, dpiId](UIElement*) {
        static bool hi = false;
        hi = !hi;
                bar.ItemText(dpiId, hi ? "DPI 150%" : "DPI 100%");
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("StatusBar", 13.0f, "textPrimary", true),
            hint,
            Row(8).Children({ btnBusy, btnReady, btnDpi }).Build(),
        }, 10.0f),
        CreateDemoSurface({ bar }, 0.0f)
    }).Build();

    return { "StatusBar 状态栏", CreatePage(
        "StatusBar 状态栏",
        "自绘多段底栏：左 / Fill / 右对齐，分隔线与进度条；不嵌套 TextBlock / ProgressBar。",
        demo) };
}
