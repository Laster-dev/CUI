#include "framework/core/CUIDsl.h"
#include "../app/ShowcaseHelpers.h"
#include "framework/controls/Flyout.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ComboBox.h"
#include "framework/style/ThemeManager.h"

namespace CUI {
using namespace CUI::DSL;

std::shared_ptr<UIElement> CreateFlyoutPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref title = CUI::Widgets::TextBlock("Flyout 弹出框展示页").Shared();
        title.FontSize(18.0f);
        title.FontWeight(CUI::FontWeight::Bold);
    
        title.Color(ThemeManager::Instance().GetColor("textPrimary"));

    CUI::Widgets::Ref desc = CUI::Widgets::TextBlock("WinUI 3 风格 Flyout 弹出窗口，支持 64ms 极速高度展开与折叠收起动画。").Shared();
        desc.FontSize(12.0f);
    
        desc.Color(ThemeManager::Instance().GetColor("textMuted"));

    CUI::Widgets::Ref btnTrigger = CUI::Widgets::Button("点击打开 Flyout 弹出框 🚀").Shared();
        btnTrigger.Width(220.0f);
        btnTrigger.Height(36.0f);

    auto flyoutContent = Column(8.0f).Children({
        std::make_shared<TextBlock>("💡 这是 Flyout 内部内容"),
        std::make_shared<TextBlock>("纯 C++ 声明式框架构建，无模糊残影。")
    }).Build();

    CUI::Widgets::Ref flyout = CUI::Widgets::Flyout(flyoutContent).Shared();

    btnTrigger->OnClick().Connect([flyout, btnTrigger](UIElement*) {
        if (flyout->IsOpen()) {
            flyout->Hide();
        } else {
            flyout->ShowAt(btnTrigger.get());
        }
    });

    auto card = Column(16.0f).Children({
        CreateShowcaseText("WinUI 3 Flyout 弹出控制", 13.0f, "textPrimary", true),
        btnTrigger,
        flyout
    }).Build();

    return Column(16.0f).Children({
        title,
        desc,
        card
    }).Build();
}

} // namespace CUI
