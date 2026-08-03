#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Flyout.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildFlyoutPage(const ShowcaseContext& ctx) {
    auto btnTrigger = std::make_shared<Button>("点击打开 Flyout 弹出框 🚀");
    btnTrigger->SetProperty("width", Value(220.0f));
    btnTrigger->SetProperty("height", Value(36.0f));

    auto flyoutContent = Column(8).Children({
        std::make_shared<TextBlock>("💡 这是 Flyout 内部内容"),
        std::make_shared<TextBlock>("纯 C++ 声明式框架构建，无模糊残影。")
    }).Build();

    auto flyout = std::make_shared<Flyout>(flyoutContent);

    btnTrigger->OnClick().Connect([flyout, btnTrigger](UIElement*) {
        if (flyout->IsOpen()) {
            flyout->Hide();
        } else {
            flyout->ShowAt(btnTrigger.get());
        }
    });

    auto demo = Column(16).Children({
        CreateDemoSurface({ btnTrigger, flyout }, 16.0f)
    }).Build();

    return { "Flyout 弹出框", CreatePage(
        "Flyout 弹出框展示页",
        "WinUI 3 风格 Flyout 弹出窗口，支持 64ms 极速高度展开与折叠收起动画。",
        demo,
        CreatePropertyGrid(ctx, btnTrigger)) };
}
