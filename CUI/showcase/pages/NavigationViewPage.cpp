#include "../app/ShowcaseHelpers.h"
#include "../../ui/framework/controls/NavigationView.h"
#include "../../ui/framework/controls/Button.h"
#include "../../ui/framework/controls/TextBlock.h"
#include "../../ui/framework/controls/ComboBox.h"
#include "../../ui/framework/window/Window.h"
#include "../../ui/framework/style/ThemeManager.h"

namespace CUI {

std::shared_ptr<UIElement> CreateNavigationViewPage(const ShowcaseContext& ctx) {
    auto title = std::make_shared<TextBlock>("NavigationView & SystemBackdrop & Theme 导航与材质系统");
    title->SetProperty("fontSize", Value(18.0f));
    title->SetProperty("fontWeight", Value("Bold"));
    title->SetProperty("theme.colorToken", Value("textPrimary"));
    title->SetProperty("color", Value(ThemeManager::Instance().GetColor("textPrimary")));

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
        std::make_shared<TextBlock>("此页配置桌面背景材质 (Backdrop) 与 明暗主题 (Theme)。")
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

    // SystemBackdrop Switcher ComboBox (Mica / MicaAlt / Acrylic / None)
    auto cbBackdrop = std::make_shared<ComboBox>();
    cbBackdrop->AddItem("Mica (云母材质)");
    cbBackdrop->AddItem("MicaAlt (沉浸云母)");
    cbBackdrop->AddItem("Acrylic (亚克力毛玻璃)");
    cbBackdrop->AddItem("None (无背景材质)");
    cbBackdrop->SetSelectedIndex(0);

    Window* win = ctx.windowRef;
    cbBackdrop->OnSelectionChanged().Connect([win](UIElement*, int index) {
        if (!win) return;
        switch (index) {
        case 0: win->SetBackdropType(BackdropType::Mica); break;
        case 1: win->SetBackdropType(BackdropType::MicaAlt); break;
        case 2: win->SetBackdropType(BackdropType::Acrylic); break;
        case 3: win->SetBackdropType(BackdropType::None); break;
        }
    });

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

    auto cardBackdrop = ControlCard("SystemBackdrop 材质与明暗主题切换", Column(12.0f).Children({
        Row(12.0f).Children({
            std::make_shared<TextBlock>("背景材质 (Backdrop):"),
            cbBackdrop
        }).Build(),
        Row(12.0f).Children({
            std::make_shared<TextBlock>("明暗主题 (Theme):"),
            btnDark,
            btnLight
        }).Build()
    }).Build());

    nav->SetProperty("width", Value(800.0f));
    nav->SetProperty("height", Value(340.0f));

    return Column(16.0f).Children({
        title,
        cardModes,
        cardBackdrop,
        nav
    }).Build();
}

} // namespace CUI
