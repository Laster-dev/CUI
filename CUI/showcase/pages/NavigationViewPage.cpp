#include "framework/core/CUIDsl.h"
#include "../app/ShowcaseHelpers.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ComboBox.h"
#include "framework/window/Window.h"
#include "framework/style/ThemeManager.h"

namespace CUI {
using namespace CUI::DSL;

std::shared_ptr<UIElement> CreateNavigationViewPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref title = CUI::Widgets::TextBlock("NavigationView & Theme 导航与主题系统").Shared();
        title.FontWeight(CUI::FontWeight::Bold);
    
        title.Color(ThemeManager::Instance().GetColor("textPrimary"));

    CUI::Widgets::Ref nav =std::make_shared<NavigationView>();
    nav.Header("CUI WinUI 3 Navigation");

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

        nav->AddMenuItem(CUI::DSL::Fluent::Control<CUI::NavigationViewItem>("首页 (Home)").Icon("🏠").Content(pageHome).Build());
        nav->AddMenuItem(CUI::DSL::Fluent::Control<CUI::NavigationViewItem>("应用 (Apps)").Icon("⚡").Content(pageApp).Build());
        nav->AddMenuItem(CUI::DSL::Fluent::Control<CUI::NavigationViewItem>("设置 (Settings)").Icon("⚙️").Content(pageSettings).Build());

    // Mode Switcher Buttons
    CUI::Widgets::Ref btnLeft = CUI::Widgets::Button("Left 侧边模式").Shared();
    CUI::Widgets::Ref btnLeftCompact = CUI::Widgets::Button("LeftCompact 紧凑模式").Shared();
    CUI::Widgets::Ref btnTop = CUI::Widgets::Button("Top 顶部模式").Shared();

    btnLeft->OnClick().Connect([nav](UIElement*) { nav.PaneDisplayMode(NavigationViewPaneDisplayMode::Left); });
    btnLeftCompact->OnClick().Connect([nav](UIElement*) { nav.PaneDisplayMode(NavigationViewPaneDisplayMode::LeftCompact); });
    btnTop->OnClick().Connect([nav](UIElement*) { nav.PaneDisplayMode(NavigationViewPaneDisplayMode::Top); });

    Window* win = ctx.windowRef;

    // Theme Switcher Buttons (Dark / Light)
    CUI::Widgets::Ref btnDark = CUI::Widgets::Button("🌙 暗色主题 (Dark)").Shared();
    CUI::Widgets::Ref btnLight = CUI::Widgets::Button("☀️ 亮色主题 (Light)").Shared();

    btnDark->OnClick().Connect([win](UIElement*) {
        if (win) win->ThemeMode(ThemeMode::Dark);
    });
    btnLight->OnClick().Connect([win](UIElement*) {
        if (win) win->ThemeMode(ThemeMode::Light);
    });

    auto cardModes = Column(10.0f).Children({
        CreateShowcaseText("NavigationView 导航模式切换", 13.0f, "textPrimary", true),
        btnLeft, btnLeftCompact, btnTop
    }).Build();

    auto cardBackdrop = Column(12.0f).Children({
        CreateShowcaseText("明暗主题切换", 13.0f, "textPrimary", true),
        Row(12.0f).Children({
            std::make_shared<TextBlock>("明暗主题:"),
            btnDark,
            btnLight
        }).Build()
    }).Build();

        nav.Width(800.0f);
        nav.Height(340.0f);

    return Column(16.0f).Children({
        title,
        cardModes,
        cardBackdrop,
        nav
    }).Build();
}

} // namespace CUI
