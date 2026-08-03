#include "../app/ShowcaseHelpers.h"
#include "../../ui/framework/controls/Flyout.h"
#include "../../ui/framework/controls/Button.h"
#include "../../ui/framework/controls/TextBlock.h"
#include "../../ui/framework/controls/ComboBox.h"

namespace CUI {

std::shared_ptr<UIElement> CreateFlyoutPage(const ShowcaseContext& ctx) {
    auto title = std::make_shared<TextBlock>("Flyout 弹出框展示页");
    title->SetProperty("fontSize", Value(18.0f));
    title->SetProperty("fontWeight", Value("Bold"));
    title->SetProperty("color", Value(D2D1::ColorF(1.0f, 1.0f, 1.0f)));

    auto desc = std::make_shared<TextBlock>("WinUI 3 风格 Flyout 弹出窗口，支持 64ms 极速高度展开与折叠收起动画。");
    desc->SetProperty("fontSize", Value(12.0f));
    desc->SetProperty("color", Value(D2D1::ColorF(0xAA / 255.0f, 0xAA / 255.0f, 0xAA / 255.0f)));

    auto btnTrigger = std::make_shared<Button>("点击打开 Flyout 弹出框 🚀");
    btnTrigger->SetProperty("width", Value(220.0f));
    btnTrigger->SetProperty("height", Value(36.0f));

    auto flyoutContent = Column(8.0f).Children({
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

    auto card = ControlCard("WinUI 3 Flyout 弹出控制", Column(16.0f).Children({
        btnTrigger,
        flyout
    }).Build());

    return Column(16.0f).Children({
        title,
        desc,
        card
    }).Build();
}

} // namespace CUI
