#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/NavigationViewItem.h"
#include <format>
#include <memory>
#include <string>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<UIElement> MakePage(const std::string& title, const std::string& body) {
    auto check = CheckboxTile("启用示例功能").Build();
    CUI::DSL::Borrow(check).State(CheckState::Checked);
    auto toggle = ToggleSwitchTile("快速设置开关", true).Build();
    return Column(12, {
        Text(title).FontSize(18.0f).FontWeight(FontWeight::SemiBold),
        Text(body),
        Row(8, {
            ElevatedButton("主要操作", [](UIElement*) {}).Build(),
            ElevatedButton("次要操作", [](UIElement*) {}).Build(),
        }),
        check,
        toggle,
        MakeStatus("此区域为 NavigationView 的内容区，随选中项切换。"),
    }).Build();
}

} // anonymous namespace

Element BuildNavigationViewPage() {
    // ---------- 1. 左侧导航（完整功能） ----------
    auto nav = std::make_shared<NavigationView>();
    DSL::Borrow(nav).PaneTitle("CUI 工作台");
    DSL::Borrow(nav).Header("首页");
    DSL::Borrow(nav).AlwaysShowHeader(true);
    DSL::Borrow(nav).PaneDisplayMode(NavigationViewPaneDisplayMode::Auto);
    DSL::Borrow(nav).IsSettingsVisible(true);
    DSL::Borrow(nav).IsBackButtonVisible(NavigationViewBackButtonVisible::Visible);
    DSL::Borrow(nav).IsBackEnabled(true);
    // 让 Auto 模式在本演示宽度下呈现展开态
    DSL::Borrow(nav).ExpandedModeThresholdWidth(600.0f);
    DSL::Borrow(nav).CompactModeThresholdWidth(480.0f);
    CUI::DSL::Borrow(nav).Width(820.0f);
    CUI::DSL::Borrow(nav).Height(430.0f);

    auto pageHome = MakePage("首页", "WinUI 3 NavigationView：PaneDisplayMode / DisplayMode / IsPaneOpen 三者分离。");
    auto pageApps = MakePage("应用", "MenuItems + FooterMenuItems + SettingsItem 共用单一选中模型。");
    auto pageDocs = MakePage("文档", "支持 Header / Separator / 单层层级 MenuItems，父节点展开子项。");
    auto pageMusic = MakePage("音乐", "LeftCompact / LeftMinimal 下 IsPaneOpen 控制 overlay / inline 行为。");
    auto pageSettings = MakePage("设置", "内置 SettingsItem，点击底部的齿轮即可进入。");
    CUI::DSL::Borrow(nav).Content(pageHome);

    auto home = std::make_shared<NavigationViewItem>("首页", "🏠");
    DSL::Borrow(home).Tag("home");
    nav->AddMenuItem(home);

    nav->AddMenuItem(std::make_shared<NavigationViewItemHeader>("资料库"));

    auto apps = std::make_shared<NavigationViewItem>("应用", "⚡");
    DSL::Borrow(apps).Tag("apps");
    nav->AddMenuItem(apps);

    auto docs = std::make_shared<NavigationViewItem>("文档", "📄");
    DSL::Borrow(docs).Tag("docs");
    DSL::Borrow(docs).SelectsOnInvoked(false); // 父节点只展开子项，不选中
    auto docsAll = std::make_shared<NavigationViewItem>("全部文件", "📁");
    DSL::Borrow(docsAll).Tag("docs-all");
    auto docsRecent = std::make_shared<NavigationViewItem>("最近使用", "🕒");
    DSL::Borrow(docsRecent).Tag("docs-recent");
    docs->AddMenuItem(docsAll);
    docs->AddMenuItem(docsRecent);
    nav->AddMenuItem(docs);

    nav->AddMenuItem(std::make_shared<NavigationViewItemSeparator>());

    auto music = std::make_shared<NavigationViewItem>("音乐", "🎵");
    DSL::Borrow(music).Tag("music");
    nav->AddMenuItem(music);

    auto account = std::make_shared<NavigationViewItem>("账户", "👤");
    DSL::Borrow(account).Tag("account");
    nav->AddFooterMenuItem(account);

    auto search = std::make_shared<TextBox>();
    CUI::DSL::Borrow(search).Placeholder("搜索…");
    CUI::DSL::Borrow(search).Height(32.0f);
    DSL::Borrow(nav).AutoSuggestBox(search);

    CUI::DSL::Borrow(nav).SelectedItem(home.get());

    auto status1 = MakeStatus("已选择 [首页]，当前显示模式: Expanded");
    nav->OnDisplayModeChanged().Connect([status1](NavigationView*, const NavigationViewDisplayModeChangedEventArgs& args) {
        status1->Text = std::format("显示模式已切换: {} (响应式自适应)",
            args.DisplayMode == NavigationViewDisplayMode::Expanded ? "Expanded" :
            args.DisplayMode == NavigationViewDisplayMode::Compact ? "Compact" : "Minimal");
    });

    auto applyTag = [nav, pageHome, pageApps, pageDocs, pageMusic, status1](const std::string& tag) {
        if (tag == "home") {
            DSL::Borrow(nav).Header("首页");
            CUI::DSL::Borrow(nav).Content(pageHome);
        } else if (tag == "apps") {
            DSL::Borrow(nav).Header("应用");
            CUI::DSL::Borrow(nav).Content(pageApps);
        } else if (tag == "docs" || tag == "docs-all" || tag == "docs-recent") {
            DSL::Borrow(nav).Header("文档");
            CUI::DSL::Borrow(nav).Content(pageDocs);
        } else if (tag == "music") {
            DSL::Borrow(nav).Header("音乐");
            CUI::DSL::Borrow(nav).Content(pageMusic);
        } else if (tag == "account") {
            DSL::Borrow(nav).Header("账户");
            CUI::DSL::Borrow(nav).Content(MakePage("账户", "FooterMenuItems 与 MenuItems 共享同一选中模型。"));
        }
        status1->Text = std::format("已导航到 [{}]，导航历史深度 {}",
                                    tag.empty() ? "设置" : tag,
                                    nav->CanGoBack() ? 1 : 0);
    };

    nav->OnItemInvoked().Connect([nav, pageSettings, applyTag](NavigationView*, const NavigationViewItemInvokedEventArgs& args) {
        if (!args.InvokedItem) return;
        if (args.IsSettingsInvoked) {
            DSL::Borrow(nav).Header("设置");
            CUI::DSL::Borrow(nav).Content(pageSettings);
            return;
        }
        applyTag(args.InvokedItem->GetTag());
    });

    // 程序化导航：NavigateTo 只负责选中 + 记录历史，内容切换需显式调用。
    auto navigateTo = [nav, applyTag](const std::string& tag) {
        if (nav->NavigateTo(tag)) {
            applyTag(tag);
        }
    };

    nav->OnBackRequested().Connect([nav, applyTag](NavigationView*) {
        nav->GoBack();
        if (NavigationViewItem* sel = nav->GetSelectedItem()) {
            applyTag(sel->GetTag());
        }
    });

    auto modeBox = ComboBoxWidget().Build();
    modeBox->AddItem("Auto（自适应）");
    modeBox->AddItem("Left（固定左侧）");
    modeBox->AddItem("LeftCompact（紧凑图标栏）");
    modeBox->AddItem("LeftMinimal（极简汉堡）");
    modeBox->AddItem("Top（顶部导航）");
    CUI::DSL::Borrow(modeBox).SelectedIndex(0);
    CUI::DSL::Borrow(modeBox).Width(260.0f);
    CUI::DSL::Borrow(modeBox).Height(32.0f);
    modeBox->OnSelectionChanged().Connect([nav](ComboBox*, int index, const std::string&) {
        static const NavigationViewPaneDisplayMode kModes[] = {
            NavigationViewPaneDisplayMode::Auto,
            NavigationViewPaneDisplayMode::Left,
            NavigationViewPaneDisplayMode::LeftCompact,
            NavigationViewPaneDisplayMode::LeftMinimal,
            NavigationViewPaneDisplayMode::Top,
        };
        if (index >= 0 && index < 5) {
            DSL::Borrow(nav).PaneDisplayMode(kModes[index]);
        }
    });

    auto btnToggle = ElevatedButton("Toggle Pane", [nav](UIElement*) { nav->TogglePane(); }).Build();
    auto chkHeader = CheckboxTile("始终显示标题").Build();
    CUI::DSL::Borrow(chkHeader).State(CheckState::Checked);
    chkHeader->OnCheckStateChanged().Connect([nav](CheckBox*, CheckState st) {
        DSL::Borrow(nav).AlwaysShowHeader(st == CheckState::Checked);
    });
    auto btnNavHome = ElevatedButton("程序化导航 → 首页", [navigateTo](UIElement*) { navigateTo("home"); }).Build();
    auto btnNavApps = ElevatedButton("程序化导航 → 应用", [navigateTo](UIElement*) { navigateTo("apps"); }).Build();

    // ---------- 2. 顶部导航模式 ----------
    auto topNav = std::make_shared<NavigationView>();
    DSL::Borrow(topNav).PaneTitle("顶部导航");
    DSL::Borrow(topNav).Header("概览");
    DSL::Borrow(topNav).AlwaysShowHeader(true);
    DSL::Borrow(topNav).PaneDisplayMode(NavigationViewPaneDisplayMode::Top);
    CUI::DSL::Borrow(topNav).Width(820.0f);
    CUI::DSL::Borrow(topNav).Height(300.0f);

    auto pageOverview = MakePage("概览", "顶部导航模式下，菜单项平铺在标题栏下方的一行中。");
    auto pageMonitor = MakePage("监控", "适合扁平化、少量顶级入口的场景。");
    auto pageReports = MakePage("报表", "切换内容时同样带有选中指示动画。");
    CUI::DSL::Borrow(topNav).Content(pageOverview);

    auto ov = std::make_shared<NavigationViewItem>("概览", "📊");
    DSL::Borrow(ov).Tag("overview");
    topNav->AddMenuItem(ov);
    auto mon = std::make_shared<NavigationViewItem>("监控", "🖥️");
    DSL::Borrow(mon).Tag("monitor");
    topNav->AddMenuItem(mon);
    auto rep = std::make_shared<NavigationViewItem>("报表", "📈");
    DSL::Borrow(rep).Tag("reports");
    topNav->AddMenuItem(rep);
    CUI::DSL::Borrow(topNav).SelectedItem(ov.get());

    auto status2 = MakeStatus("顶部导航模式：菜单平铺，选中指示条滑动切换。");
    topNav->OnItemInvoked().Connect([topNav, pageOverview, pageMonitor, pageReports, status2](
        NavigationView*, const NavigationViewItemInvokedEventArgs& args) {
        if (!args.InvokedItem) return;
        const std::string& tag = args.InvokedItem->GetTag();
        if (tag == "overview") {
            DSL::Borrow(topNav).Header("概览");
            CUI::DSL::Borrow(topNav).Content(pageOverview);
        } else if (tag == "monitor") {
            DSL::Borrow(topNav).Header("监控");
            CUI::DSL::Borrow(topNav).Content(pageMonitor);
        } else if (tag == "reports") {
            DSL::Borrow(topNav).Header("报表");
            CUI::DSL::Borrow(topNav).Content(pageReports);
        }
        status2->Text = std::format("已切换到 [{}]", tag);
    });

    SamplePageSpec spec;
    spec.title = "NavigationView (导航视图)";
    spec.subtitle = "WinUI 3 风格主导航框架：PaneDisplayMode / DisplayMode / IsPaneOpen 分离，支持层级菜单、分组头、分隔线、底部设置项与返回导航历史。";
    spec.sections = {
        {
            "左侧导航视图（Auto / Left 自适应）",
            "1. 菜单支持 Header 分组、Separator 分隔线与单层子菜单（父节点只展开不选中）；\n"
            "2. 底部设置项（⚙）与 Footer 菜单项共用选中模型；\n"
            "3. 顶部搜索框 (AutoSuggestBox) 常驻；返回按钮记录程序化导航历史；\n"
            "4. 下拉框切换五种 PaneDisplayMode，Toggle Pane 控制开合，观察响应式动画。",
            Column(12, {
                Row(8, { modeBox, btnToggle, chkHeader, btnNavHome, btnNavApps }),
                nav,
                status1,
            }),
        },
        {
            "顶部导航模式（Top）",
            "菜单项平铺在标题栏下方一行，选中指示条平滑滑动；适合扁平化少量顶级入口的场景。",
            Column(12, {
                topNav,
                status2,
            }),
        },
    };

    spec.source = R"(
// 1) 创建导航视图
auto nav = std::make_shared<NavigationView>();
DSL::Borrow(nav).PaneTitle("工作台");
DSL::Borrow(nav).Header("首页");
DSL::Borrow(nav).PaneDisplayMode(NavigationViewPaneDisplayMode::Auto);

// 2) 添加菜单项（含层级与分组）
auto home = std::make_shared<NavigationViewItem>("首页", "🏠");
DSL::Borrow(home).Tag("home");
nav->AddMenuItem(home);
nav->AddMenuItem(std::make_shared<NavigationViewItemHeader>("资料库"));

auto docs = std::make_shared<NavigationViewItem>("文档", "📄");
DSL::Borrow(docs).Tag("docs");
docs->AddMenuItem(std::make_shared<NavigationViewItem>("全部文件", "📁"));
nav->AddMenuItem(docs);

// 3) 内容切换
nav->OnItemInvoked().Connect([](NavigationView*, const NavigationViewItemInvokedEventArgs& args) {
    // args.InvokedItem->GetTag() 定位目标页面
});

// 4) 程序化导航（记录历史，支持返回）
nav->NavigateTo("home");
nav->OnBackRequested().Connect([](NavigationView*) { nav->GoBack(); });
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery


