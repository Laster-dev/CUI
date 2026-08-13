#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/ToastCenter.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildToastPage(const ShowcaseContext& ctx) {
    auto log = CreateShowcaseText("[就绪] 点击上方按钮触发 Toast 通知...", 12.0f, "#B5CEA8", false, "Consolas");
    auto count = CreateShowcaseText("活动通知: 0", 12.0f, "#9CDCFE", false, "Consolas");
    auto updateCount = [window = ctx.windowRef, count]() {
        auto center = ToastCenter::Ensure(window->GetRootElement().get());
        count->SetText("活动通知: " + std::to_string(center ? center->GetActiveCount() : 0));
    };
    auto emitToast = [window = ctx.windowRef, log, updateCount](const std::string& title, const std::string& message, ToastType type, ToastCorner corner, int durationMs) {
        ToastCenter::Show(window->GetRootElement().get(), title, message, type, corner, durationMs);
        log->SetText("[Toast] " + title + " -> " + message);
        updateCount();
    };

    auto info = ElevatedButton("信息 Info", [emitToast](UIElement*) { emitToast("Info", "信息提示消息", ToastType::Info, ToastCorner::BottomRight, 2200); }).Background(Rgb(0x007ACC)).Padding(14, 8, 14, 8).CornerRadius(4).Build();
    auto success = ElevatedButton("成功 Success", [emitToast](UIElement*) { emitToast("Success", "操作成功！", ToastType::Success, ToastCorner::BottomRight, 2200); }).Background(Rgb(0x10B981)).Padding(14, 8, 14, 8).CornerRadius(4).Build();
    auto warn = ElevatedButton("警告 Warning", [emitToast](UIElement*) { emitToast("Warning", "警告提醒消息", ToastType::Warning, ToastCorner::BottomRight, 2200); }).Background(Rgb(0xD7A400)).Padding(14, 8, 14, 8).CornerRadius(4).Build();
    auto error = ElevatedButton("错误 Error", [emitToast](UIElement*) { emitToast("Error", "错误异常消息", ToastType::Error, ToastCorner::BottomRight, 2200); }).Background(Rgb(0xD13438)).Padding(14, 8, 14, 8).CornerRadius(4).Build();
    auto fromTemplate = ElevatedButton("按模板弹出").Background(Rgb(0x8E44AD)).Padding(14, 8, 14, 8).CornerRadius(4).Build();
    fromTemplate->OnClick().Connect([window = ctx.windowRef, tmpl = ctx.toastTemplate, log, updateCount](UIElement*) {
        auto center = ToastCenter::Ensure(window->GetRootElement().get());
        if (center) center->ShowFromTemplate(tmpl.get(), "Template Toast", "这是声明式 Widget 版本模板通知");
        log->SetText("[Toast] 已按模板生成一条通知。");
        updateCount();
    });
    auto dismiss = ElevatedButton("全部关闭").Background(Rgb(0x5A5A5A)).Padding(14, 8, 14, 8).CornerRadius(4).Build();
    dismiss->OnClick().Connect([window = ctx.windowRef, log, updateCount](UIElement*) {
        auto center = ToastCenter::Ensure(window->GetRootElement().get());
        if (center) center->DismissAll();
        log->SetText("[Toast] 已请求关闭全部通知。");
        updateCount();
    });

    auto demo = Column(14).Children({
        CreateDemoSurface({
            CreateShowcaseText("快速触发", 13.0f, "#4EC9B0", true),
            Row(10).Children({ info, success, warn, error }).Build(),
            Row(10).Children({ fromTemplate, dismiss }).Build()
        }, 12.0f),
        CreateDemoSurface({
            CreateShowcaseText("事件日志 (Event Log)", 11.0f, "#4EC9B0", true),
            log,
            count
        }, 4.0f)
    }).Build();

    return { "Toast 通知中心", CreatePage(
        "Toast 通知中心 / Toast Notification Center",
        "声明式 Widget + 入场/出场动画 + 多条堆叠通知。",
        demo) };
}
