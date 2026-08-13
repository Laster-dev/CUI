#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Flyout.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildFlyoutPage(const ShowcaseContext& ctx) {
    auto btnTrigger = std::make_shared<Button>("打开 Flyout");
    btnTrigger->SetWidth(160.0f);
    btnTrigger->SetHeight(36.0f);
    btnTrigger->SetCornerRadius(4.0f);

    auto flyoutContent = Column(10).Children({
        CreateShowcaseText("Flyout 内容", 14.0f, "textPrimary", true),
        CreateShowcaseText("点击外部或再次点击按钮可关闭。", 12.0f, "textSecondary", false),
        ElevatedButton("操作按钮").Padding(12, 6, 12, 6).Build()
    }).Build();

    auto flyout = std::make_shared<Flyout>(flyoutContent);
    flyout->SetPlacement(FlyoutPlacement::Bottom);

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
        "Flyout 弹出框",
        "浮层不参与文档流布局；背景始终不透明，不受系统材质 alpha 影响。点击外部关闭。",
        demo) };
}
