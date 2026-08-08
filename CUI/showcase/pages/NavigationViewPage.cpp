#include "../app/ShowcaseHelpers.h"
#include "../../ui/framework/controls/NavigationView.h"
#include "../../ui/framework/controls/Button.h"
#include "../../ui/framework/controls/TextBlock.h"
#include "../../ui/framework/controls/ComboBox.h"
#include "../../ui/framework/window/Window.h"
#include "../../ui/framework/style/ThemeManager.h"

namespace CUI {

std::shared_ptr<UIElement> CreateNavigationViewPage(const ShowcaseContext& ctx) {
    auto title = std::make_shared<TextBlock>("NavigationView & Theme 导航与主题系统");
    title->SetFontSize(18.0f);
    title->SetFontWeight("Bold");
    title->SetColorToken(ThemeTokenId::TextPrimary);
    title->SetColor(ThemeManager::Instance().GetColor("textPrimary"));

    auto nav = std::make_shared<NavigationView>();
    nav->SetHeader("CUI WinUI 3 Navigation");

    auto pageHome = Column(12.0f).Children({
        std::make_shared<TextBlock>("🏠 首页 (Home Page)"),
        std::make_shared<TextBlock>("欢迎使用 CUI WinUI 3 风格 NavigationView 视图控件！")
    }).Build();

    auto pageApp = Column(12.0f).Children({
        std::make_shared<TextBlock>("⚡ 应用 (Apps Page)"),
        std::make_shared<TextBlock>("支持 Left / LeftCompact / Top 模式切换。")
    }).Build();

    auto pageSettings = Column(12.0f).Children({
        std::make_shared<TextBlock>("⚙️ 设置 (Settings Page)"),
        std::make_shared<TextBlock>("此页配置明暗主题 (Theme)。")
    }).Build();

    nav->AddItem("home", "首页 (Home)", "🏠", pageHome);
    nav->AddItem("apps", "应用 (Apps)", "⚡", pageApp);
    nav->AddItem("settings", "设置 (Settings)", "⚙️", pageSettings);

    // Mode Switcher Buttons
    auto btnLeft = std::make_shared<Button>("Left 侧边模式");
    auto btnLeftCompact = std::make_shared<Button>("LeftCompact 紧凑模式");
    auto btnTop = std::make_shared<Button>("Top 顶部模式");

    btnLeft->OnClick().Connect([nav](UIElement*) { nav->SetPaneDisplayMode(NavigationViewPaneDisplayMode::Left); });
    btnLeftCompact->OnClick().Connect([nav](UIElement*) { nav->SetPaneDisplayMode(NavigationViewPaneDisplayMode::LeftCompact); });
    btnTop->OnClick().Connect([nav](UIElement*) { nav->SetPaneDisplayMode(NavigationViewPaneDisplayMode::Top); });

    Window* win = ctx.windowRef;

    // Theme Switcher Buttons (Dark / Light)
    auto btnDark = std::make_shared<Button>("🌙 暗色主题 (Dark)");
    auto btnLight = std::make_shared<Button>("☀️ 亮色主题 (Light)");

    btnDark->OnClick().Connect([win](UIElement*) {
        if (win) win->SetThemeMode(ThemeMode::Dark);
    });
    btnLight->OnClick().Connect([win](UIElement*) {
        if (win) win->SetThemeMode(ThemeMode::Light);
    });

    auto cardModes = ControlCard("NavigationView 导航模式切换", Row(10.0f).Children({
        btnLeft, btnLeftCompact, btnTop
    }).Build());

    auto cardBackdrop = ControlCard("明暗主题切换", Column(12.0f).Children({
        Row(12.0f).Children({
            std::make_shared<TextBlock>("明暗主题:"),
            btnDark,
            btnLight
        }).Build()
    }).Build());

    nav->SetWidth(800.0f);
    nav->SetHeight(340.0f);

    return Column(16.0f).Children({
        title,
        cardModes,
        cardBackdrop,
        nav
    }).Build();
}

} // namespace CUI
