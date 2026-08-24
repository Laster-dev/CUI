#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/StatusBar.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildStatusBarPage(const ShowcaseContext& ctx) {
    auto bar = std::make_shared<StatusBar>();
    CUI::DSL::Borrow(bar).Width(-1.0f);
    CUI::DSL::Borrow(bar).Height(26.0f);

    const int msgId = bar->AddTextItem("就绪", StatusBarItemAlignment::Left);
    DSL::Borrow(bar).ItemIcon(msgId, "●");
    bar->AddTextItem("CUI Control Gallery", StatusBarItemAlignment::Fill);
    bar->AddSeparator(StatusBarItemAlignment::Right);
    const int dpiId = bar->AddTextItem("DPI 100%", StatusBarItemAlignment::Right);
    bar->AddSeparator(StatusBarItemAlignment::Right);
    const int zoomId = bar->AddTextItem("缩放 100%", StatusBarItemAlignment::Right);
    bar->AddSeparator(StatusBarItemAlignment::Right);
    const int progId = bar->AddProgressItem("索引", StatusBarItemAlignment::Right, 140.0f);
    DSL::Borrow(bar).ItemProgress(progId, 0.35f);

    auto hint = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("底栏：左消息 + 中间弹性标题 + 右侧 DPI / 缩放 / 进度（纯自绘，无 TextBlock 子控件）。", 12.0f, "textSecondary", false));

    auto btnBusy = std::make_shared<Button>("模拟忙碌");
    btnBusy->OnClick().Connect([bar, msgId, progId](UIElement*) {
        DSL::Borrow(bar).ItemText(msgId, "正在处理…");
        DSL::Borrow(bar).ItemProgress(progId, 0.72f);
    });
    auto btnReady = std::make_shared<Button>("恢复就绪");
    btnReady->OnClick().Connect([bar, msgId, progId, dpiId, zoomId](UIElement*) {
        DSL::Borrow(bar).ItemText(msgId, "就绪");
        DSL::Borrow(bar).ItemProgress(progId, 0.35f);
        DSL::Borrow(bar).ItemText(dpiId, "DPI 100%");
        DSL::Borrow(bar).ItemText(zoomId, "缩放 100%");
    });
    auto btnDpi = std::make_shared<Button>("切换 DPI 文案");
    btnDpi->OnClick().Connect([bar, dpiId](UIElement*) {
        static bool hi = false;
        hi = !hi;
        DSL::Borrow(bar).ItemText(dpiId, hi ? "DPI 150%" : "DPI 100%");
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
