#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Toast.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildButtonPage(const ShowcaseContext& ctx) {
    auto target = ElevatedButton("交互测试按钮").Background("#007ACC").HoverBackground("#0098FF").PressedBackground("#005A9E").FontSize(14).Padding(16, 8, 16, 8).CornerRadius(4).Build();
    auto log = CreateShowcaseText("[就绪] 点击目标按钮触发 OnClick 点击事件...", 12.0f, "#B5CEA8", false, "Consolas");
    target->OnClick().Connect([window = ctx.windowRef, log](UIElement*) {
        log->SetProperty("text", Value("[事件] OnClick 已触发，按钮交互链路正常。"));
        Toast::Show(window->GetRootElement().get(), "Button", "按钮点击触发 OnClick 事件！", ToastCorner::BottomRight, 2200);
    });

    auto demo = Column(16).Children({
        CreateDemoSurface({ target }, 0.0f),
        Column(4).Background("#252526").Padding(10).CornerRadius(4).Border("#333333", 1).Children({
            CreateShowcaseText("事件日志 (Event Log)", 11.0f, "#4EC9B0", true),
            log
        }).Build()
    }).Build();

    return { "Button 按钮", CreatePage(
        "Button 按钮控件全属性交互控制台",
        "由 PropertyGrid 自动化反射引擎进行 100% 精准双向绑定控制。",
        demo,
        CreatePropertyGrid(ctx, target)) };
}
