#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/NavigationViewItem.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ComboBox.h"
#include "framework/controls/CheckBox.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/ToggleSwitch.h"
#include "framework/controls/ListBox.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {

std::shared_ptr<UIElement> MakePage(const std::string& title, const std::string& body, int variant) {
    auto chk = Fluent::Control<CheckBox>().Build();
    ElementBuilder<CheckBox>(chk).State((variant % 2 == 0) ? CheckState::Checked : CheckState::Unchecked);

    auto toggle = Fluent::Control<ToggleSwitch>().Build();
    ElementBuilder<ToggleSwitch>(toggle).Header(variant % 2 == 0 ? "ToggleSwitch" : "Quick Toggle").IsOn(variant % 2 == 0);

    auto combo = Fluent::Control<ComboBox>().Build();
    ElementBuilder<ComboBox>(combo).Width(240.0f);
    ElementBuilder<ComboBox>(combo).Height(32.0f);
    ElementBuilder<ComboBox>(combo).AddItem(variant % 2 == 0 ? "WinUI-like" : "CUI-custom");
    ElementBuilder<ComboBox>(combo).AddItem("Dark/Light");
    ElementBuilder<ComboBox>(combo).AddItem("Accent");
    ElementBuilder<ComboBox>(combo).SelectedIndex(variant % 3);

    auto list = std::make_shared<ListBox>();
    ElementBuilder<ListBox>(list).Height(120.0f);
    CUI::DSL::Borrow(list).ItemHeight(28.0f);
    DSL::Borrow(list).SelectionMode(ListBoxSelectionMode::Single);
    list->AddItem(variant % 2 == 0 ? "One" : "Alpha");
    list->AddItem(variant % 2 == 0 ? "Two" : "Beta");
    list->AddItem(variant % 2 == 0 ? "Three" : "Gamma");
    ElementBuilder<ListBox>(list).SelectedIndex(0);

    auto input = std::make_shared<TextBox>("Type something...");
    ElementBuilder<TextBox>(input).Width(360.0f);

    return Column(12).Children({
        Fluent::TextBlock(title).Build(),
        Fluent::TextBlock(body).Build(),
        chk,
        toggle,
        combo,
        list,
        input
    }).Build();
}

} // namespace

ShowcasePage BuildNavigationViewPage(const ShowcaseContext& ctx) {
    auto nav = Fluent::Control<NavigationView>().Build();
    ElementBuilder<NavigationView>(nav).PaneTitle("CUI");
    ElementBuilder<NavigationView>(nav).Header("Home");
    ElementBuilder<NavigationView>(nav).AlwaysShowHeader(true);
    ElementBuilder<NavigationView>(nav).PaneDisplayMode(NavigationViewPaneDisplayMode::Auto);
    ElementBuilder<NavigationView>(nav).IsSettingsVisible(true);

    auto pageHome = MakePage("Home", "WinUI 3 NavigationView: PaneDisplayMode / DisplayMode / IsPaneOpen 三者分离。", 0);
    auto pageApps = MakePage("Apps", "MenuItems + FooterMenuItems + SettingsItem 共用单一选中模型。", 1);
    auto pageDocs = MakePage("Documents", "支持 Header / Separator / 一层层级 MenuItems。", 2);
    auto pageMusic = MakePage("Music", "LeftCompact / LeftMinimal 下 IsPaneOpen 控制 overlay/inline 行为。", 3);
    auto pageSettings = MakePage("Settings", "内置 SettingsItem；ItemInvoked → SelectionChanged。", 4);

    ElementBuilder<NavigationView>(nav).Content(pageHome);

    // MenuItems
    auto home = Fluent::Control<NavigationViewItem>("Home", "🏠").Build();
    ElementBuilder<NavigationViewItem>(home).Tag("home");
    ElementBuilder<NavigationView>(nav).AddMenuItem(home);

    ElementBuilder<NavigationView>(nav).AddMenuItem(Fluent::Control<NavigationViewItemHeader>("Library").Build()).Build();

    auto apps = Fluent::Control<NavigationViewItem>("Apps", "⚡").Build();
    ElementBuilder<NavigationViewItem>(apps).Tag("apps");
    ElementBuilder<NavigationView>(nav).AddMenuItem(apps);

    auto docs = Fluent::Control<NavigationViewItem>("Documents", "📄").Build();
    ElementBuilder<NavigationViewItem>(docs).Tag("docs");
    // Hierarchy: parent does not select; expands children.
    ElementBuilder<NavigationViewItem>(docs).SelectsOnInvoked(false);
    auto docsAll = Fluent::Control<NavigationViewItem>("All files", "📁").Build();
    ElementBuilder<NavigationViewItem>(docsAll).Tag("docs-all");
    auto docsRecent = Fluent::Control<NavigationViewItem>("Recent", "🕒").Build();
    ElementBuilder<NavigationViewItem>(docsRecent).Tag("docs-recent");
    ElementBuilder<NavigationViewItem>(docs).AddNestedItem(docsAll);
    ElementBuilder<NavigationViewItem>(docs).AddNestedItem(docsRecent);
    ElementBuilder<NavigationView>(nav).AddMenuItem(docs);

    ElementBuilder<NavigationView>(nav).AddMenuItem(Fluent::Control<NavigationViewItemSeparator>().Build());

    auto music = Fluent::Control<NavigationViewItem>("Music", "🎵").Build();
    ElementBuilder<NavigationViewItem>(music).Tag("music");
    ElementBuilder<NavigationView>(nav).AddMenuItem(music);

    // Footer
    auto account = Fluent::Control<NavigationViewItem>("Account", "👤").Build();
    ElementBuilder<NavigationViewItem>(account).Tag("account");
    ElementBuilder<NavigationView>(nav).AddFooterMenuItem(account);

    // AutoSuggest slot
    auto search = Fluent::TextBox();
    ElementBuilder<TextBox>(search).Placeholder("Search").Height(32.0f);
    CUI::DSL::Borrow(search).Height(32.0f);
    ElementBuilder<NavigationView>(nav).AutoSuggestBox(search);

    ElementBuilder<NavigationView>(nav).SelectedItem(home.get());

    ElementBuilder<NavigationView>(nav).OnNavigationItemInvoked([nav, pageHome, pageApps, pageDocs, pageMusic, pageSettings,
                                  docsAll, docsRecent](NavigationView*, const NavigationViewItemInvokedEventArgs& args) {
        if (!args.InvokedItem) {
            return;
        }
        if (args.IsSettingsInvoked) {
            ElementBuilder<NavigationView>(nav).Header("Settings");
            ElementBuilder<NavigationView>(nav).Content(pageSettings);
            return;
        }
        const std::string& tag = args.InvokedItem->GetTag();
        if (tag == "home") {
            ElementBuilder<NavigationView>(nav).Header("Home");
            ElementBuilder<NavigationView>(nav).Content(pageHome);
        } else if (tag == "apps") {
            ElementBuilder<NavigationView>(nav).Header("Apps");
            ElementBuilder<NavigationView>(nav).Content(pageApps);
        } else if (tag == "docs" || tag == "docs-all" || tag == "docs-recent") {
            ElementBuilder<NavigationView>(nav).Header("Documents");
            ElementBuilder<NavigationView>(nav).Content(pageDocs);
        } else if (tag == "music") {
            ElementBuilder<NavigationView>(nav).Header("Music");
            ElementBuilder<NavigationView>(nav).Content(pageMusic);
        } else if (tag == "account") {
            ElementBuilder<NavigationView>(nav).Header("Account");
            ElementBuilder<NavigationView>(nav).Content(MakePage("Account", "FooterMenuItems 与 MenuItems 共享选中。", 5));
        }
        (void)docsAll;
        (void)docsRecent;
    });

    // PaneDisplayMode switcher
    auto modeBox = Fluent::Control<ComboBox>().Build();
    modeBox->AddItem("Auto");
    modeBox->AddItem("Left");
    modeBox->AddItem("LeftCompact");
    modeBox->AddItem("LeftMinimal");
    modeBox->AddItem("Top");
    CUI::DSL::Borrow(modeBox).SelectedIndex(0);
    modeBox->OnSelectionChanged().Connect([nav](ComboBox* box, int index, const std::string&) {
        static const NavigationViewPaneDisplayMode kModes[] = {
            NavigationViewPaneDisplayMode::Auto,
            NavigationViewPaneDisplayMode::Left,
            NavigationViewPaneDisplayMode::LeftCompact,
            NavigationViewPaneDisplayMode::LeftMinimal,
            NavigationViewPaneDisplayMode::Top
        };
        if (index >= 0 && index < 5) {
            ElementBuilder<NavigationView>(nav).PaneDisplayMode(kModes[index]);
        }
        (void)box;
    });

    auto chkHeader = Fluent::Control<CheckBox>().Build();
    ElementBuilder<CheckBox>(chkHeader).Text("AlwaysShowHeader");
    CUI::DSL::Borrow(chkHeader).State(CheckState::Checked);
    chkHeader->OnCheckStateChanged().Connect([nav](CheckBox*, CheckState state) {
        ElementBuilder<NavigationView>(nav).AlwaysShowHeader(state == CheckState::Checked);
    });

    auto btnToggle = Fluent::Button("Toggle Pane").Build();
    btnToggle->OnClick().Connect([nav](UIElement*) {
        nav->TogglePane();
    });

    auto modeLabel = Fluent::TextBlock("PaneDisplayMode").Build();
    ElementBuilder<NavigationView>(nav).Width(860.0f);
    ElementBuilder<NavigationView>(nav).Height(420.0f);

    auto demo = Column(12).Children({
        Row(12).Children({ modeLabel, modeBox, btnToggle, chkHeader }).Build(),
        nav
    }).Build();

    return { "NavigationView 导航", CreatePage(
        "WinUI 3 NavigationView",
        "PaneDisplayMode / DisplayMode / IsPaneOpen 分离；MenuItems · Footer · Settings · Header/Separator · 层级 · Top。",
        demo) };
}

