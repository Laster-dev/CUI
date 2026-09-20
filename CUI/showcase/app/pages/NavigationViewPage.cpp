#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 CUI::Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
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
    CUI::Widgets::Ref chk = CUI::Widgets::CheckBox().Shared();
    chk.State((variant % 2 == 0) ? CheckState::Checked : CheckState::Unchecked);

    CUI::Widgets::Ref toggle = CUI::Widgets::ToggleSwitch().Shared();
    toggle.Header(variant % 2 == 0 ? "ToggleSwitch" : "Quick Toggle");
    toggle.IsOn(variant % 2 == 0);

    CUI::Widgets::Ref combo = CUI::Widgets::ComboBox().Shared();
    combo.Width(240.0f);
    combo.Height(32.0f);
    combo->AddItem(variant % 2 == 0 ? "WinUI-like" : "CUI-custom");
    combo->AddItem("Dark/Light");
    combo->AddItem("Accent");
    combo.SelectedIndex(variant % 3);

    CUI::Widgets::Ref list = CUI::Widgets::ListBox().Shared();
    list.Height(120.0f);
        list.ItemHeight(28.0f);
        list.SelectionMode(ListBoxSelectionMode::Single);
    list->AddItem(variant % 2 == 0 ? "One" : "Alpha");
    list->AddItem(variant % 2 == 0 ? "Two" : "Beta");
    list->AddItem(variant % 2 == 0 ? "Three" : "Gamma");
    list.SelectedIndex(0);

    CUI::Widgets::Ref input = CUI::Widgets::TextBox("Type something...").Shared();
    input.Width(360.0f);

    return Column(12).Children({
        CUI::Widgets::TextBlock(title).Shared(),
        CUI::Widgets::TextBlock(body).Shared(),
        chk,
        toggle,
        combo,
        list,
        input
    }).Build();
}

} // namespace

ShowcasePage BuildNavigationViewPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref nav = CUI::Widgets::NavigationView().Shared();
    nav.PaneTitle("CUI");
    nav.Header("Home");
    nav.AlwaysShowHeader(true);
    nav.PaneDisplayMode(NavigationViewPaneDisplayMode::Auto);
    nav.IsSettingsVisible(true);

    auto pageHome = MakePage("Home", "WinUI 3 NavigationView: PaneDisplayMode / DisplayMode / IsPaneOpen 三者分离。", 0);
    auto pageApps = MakePage("Apps", "MenuItems + FooterMenuItems + SettingsItem 共用单一选中模型。", 1);
    auto pageDocs = MakePage("Documents", "支持 Header / Separator / 一层层级 MenuItems。", 2);
    auto pageMusic = MakePage("Music", "LeftCompact / LeftMinimal 下 IsPaneOpen 控制 overlay/inline 行为。", 3);
    auto pageSettings = MakePage("Settings", "内置 SettingsItem；ItemInvoked → SelectionChanged。", 4);

    nav.Content(pageHome);

    // MenuItems
    CUI::Widgets::Ref home = CUI::Widgets::NavigationViewItem("Home", "🏠").Shared();
    home.Tag("home");
    nav->AddMenuItem(home);

    nav->AddMenuItem(CUI::Widgets::NavigationViewItemHeader("Library").Shared());

    CUI::Widgets::Ref apps = CUI::Widgets::NavigationViewItem("Apps", "⚡").Shared();
    apps.Tag("apps");
    nav->AddMenuItem(apps);

    CUI::Widgets::Ref docs = CUI::Widgets::NavigationViewItem("Documents", "📄").Shared();
    docs.Tag("docs");
    // Hierarchy: parent does not select; expands children.
    docs.SelectsOnInvoked(false);
    CUI::Widgets::Ref docsAll = CUI::Widgets::NavigationViewItem("All files", "📁").Shared();
    docsAll.Tag("docs-all");
    CUI::Widgets::Ref docsRecent = CUI::Widgets::NavigationViewItem("Recent", "🕒").Shared();
    docsRecent.Tag("docs-recent");
    docs->AddMenuItem(docsAll);
    docs->AddMenuItem(docsRecent);
    nav->AddMenuItem(docs);

    nav->AddMenuItem(CUI::Widgets::NavigationViewItemSeparator().Shared());

    CUI::Widgets::Ref music = CUI::Widgets::NavigationViewItem("Music", "🎵").Shared();
    music.Tag("music");
    nav->AddMenuItem(music);

    // Footer
    CUI::Widgets::Ref account = CUI::Widgets::NavigationViewItem("Account", "👤").Shared();
    account.Tag("account");
    nav->AddFooterMenuItem(account);

    // AutoSuggest slot
    CUI::Widgets::Ref search = CUI::Widgets::TextBox().Shared();
    search.Placeholder("Search");
        search.Height(32.0f);
    nav.AutoSuggestBox(search);

    nav.SelectedItem(home.get());

    nav->OnItemInvoked().Connect([nav, pageHome, pageApps, pageDocs, pageMusic, pageSettings,
                                  docsAll, docsRecent](NavigationView*, const NavigationViewItemInvokedEventArgs& args) {
        if (!args.InvokedItem) {
            return;
        }
        if (args.IsSettingsInvoked) {
            nav.Header("Settings");
            nav.Content(pageSettings);
            return;
        }
        const std::string& tag = args.InvokedItem->GetTag();
        if (tag == "home") {
            nav.Header("Home");
            nav.Content(pageHome);
        } else if (tag == "apps") {
            nav.Header("Apps");
            nav.Content(pageApps);
        } else if (tag == "docs" || tag == "docs-all" || tag == "docs-recent") {
            nav.Header("Documents");
            nav.Content(pageDocs);
        } else if (tag == "music") {
            nav.Header("Music");
            nav.Content(pageMusic);
        } else if (tag == "account") {
            nav.Header("Account");
            nav.Content(MakePage("Account", "FooterMenuItems 与 MenuItems 共享选中。", 5));
        }
        (void)docsAll;
        (void)docsRecent;
    });

    // PaneDisplayMode switcher
    CUI::Widgets::Ref modeBox = CUI::Widgets::ComboBox().Shared();
    modeBox->AddItem("Auto");
    modeBox->AddItem("Left");
    modeBox->AddItem("LeftCompact");
    modeBox->AddItem("LeftMinimal");
    modeBox->AddItem("Top");
        modeBox.SelectedIndex(0);
    modeBox->OnSelectionChanged().Connect([nav](ComboBox* box, int index, const std::string&) {
        static const NavigationViewPaneDisplayMode kModes[] = {
            NavigationViewPaneDisplayMode::Auto,
            NavigationViewPaneDisplayMode::Left,
            NavigationViewPaneDisplayMode::LeftCompact,
            NavigationViewPaneDisplayMode::LeftMinimal,
            NavigationViewPaneDisplayMode::Top
        };
        if (index >= 0 && index < 5) {
            nav.PaneDisplayMode(kModes[index]);
        }
        (void)box;
    });

    CUI::Widgets::Ref chkHeader = CUI::Widgets::CheckBox("AlwaysShowHeader").Shared();
        chkHeader.State(CheckState::Checked);
    chkHeader->OnCheckStateChanged().Connect([nav](CheckBox*, CheckState state) {
        nav.AlwaysShowHeader(state == CheckState::Checked);
    });

    CUI::Widgets::Ref btnToggle = CUI::Widgets::Button("Toggle Pane").Shared();
    btnToggle->OnClick().Connect([nav](UIElement*) {
        nav->TogglePane();
    });

    CUI::Widgets::Ref modeLabel = CUI::Widgets::TextBlock("PaneDisplayMode").Shared();
    nav.Width(860.0f);
    nav.Height(420.0f);

    auto demo = Column(12).Children({
        Row(12).Children({ modeLabel, modeBox, btnToggle, chkHeader }).Build(),
        nav
    }).Build();

    return { "NavigationView 导航", CreatePage(
        "WinUI 3 NavigationView",
        "PaneDisplayMode / DisplayMode / IsPaneOpen 分离；MenuItems · Footer · Settings · Header/Separator · 层级 · Top。",
        demo) };
}

