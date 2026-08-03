#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/CheckBox.h"
#include "framework/window/Window.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildNavigationViewPage(const ShowcaseContext& ctx) {
    auto nav = std::make_shared<NavigationView>();
    nav->SetHeader("CUI WinUI 3 Navigation");

    auto pageHome = Column(12).Children({
        std::make_shared<TextBlock>("🏠 首页 (Home Page)"),
        std::make_shared<TextBlock>("欢迎使用 CUI WinUI 3 风格 NavigationView 视图控件！")
    }).Build();

    auto pageApp = Column(12).Children({
        std::make_shared<TextBlock>("⚡ 应用 (Apps Page)"),
        std::make_shared<TextBlock>("点击左上角 ☰ 按钮可自由切换展开/折叠面板。")
    }).Build();

    auto pageSettings = Column(12).Children({
        std::make_shared<TextBlock>("⚙️ 设置 (Settings Page)"),
        std::make_shared<TextBlock>("支持设置点击菜单项后【自动收起】面板。")
    }).Build();

    nav->AddItem("home", "首页 (Home)", "🏠", pageHome);
    nav->AddItem("apps", "应用 (Apps)", "⚡", pageApp);
    nav->AddItem("settings", "设置 (Settings)", "⚙️", pageSettings);

    // Auto-Collapse CheckBox
    auto chkAutoCollapse = std::make_shared<CheckBox>();
    chkAutoCollapse->SetProperty("text", Value("点击菜单项后自动收起 (Auto-Collapse)"));
    chkAutoCollapse->SetState(CheckState::Unchecked);
    chkAutoCollapse->OnCheckStateChanged().Connect([nav](CheckBox*, CheckState state) {
        nav->SetAutoCollapse(state == CheckState::Checked);
    });

    // Manual Toggle Button
    auto btnToggle = std::make_shared<Button>("☰ 切换展开/折叠 (Toggle Pane)");
    btnToggle->OnClick().Connect([nav](UIElement*) {
        nav->TogglePane();
    });

    nav->SetProperty("width", Value(800.0f));
    nav->SetProperty("height", Value(340.0f));

    auto demo = Column(16).Children({
        Row(12).Children({ btnToggle, chkAutoCollapse }).Build(),
        nav
    }).Build();

    return { "NavigationView 导航", CreatePage(
        "NavigationView 侧边导航视图与自动收起控制",
        "整合 WinUI 3 侧边栏导航，支持左上角 ☰ 展开/折叠面板与【自动收起】逻辑。",
        demo,
        CreatePropertyGrid(ctx, nav)) };
}
