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
    auto chk = std::make_shared<CheckBox>();
    chk->SetText(variant % 2 == 0 ? "启用示例功能" : "允许用户输入");
    chk->SetState((variant % 2 == 0) ? CheckState::Checked : CheckState::Unchecked);

    auto toggle = std::make_shared<ToggleSwitch>();
    toggle->SetHeader(variant % 2 == 0 ? "ToggleSwitch" : "Quick Toggle");
    toggle->SetIsOn(variant % 2 == 0);

    auto combo = std::make_shared<ComboBox>();
    combo->SetWidth(240.0f);
    combo->SetHeight(32.0f);
    combo->AddItem(variant % 2 == 0 ? "WinUI-like" : "CUI-custom");
    combo->AddItem("Dark/Light");
    combo->AddItem("Accent");
    combo->SetSelectedIndex(variant % 3);

    auto list = std::make_shared<ListBox>();
    list->SetHeight(120.0f);
    list->SetItemHeight(28.0f);
    list->SetSelectionMode(ListBoxSelectionMode::Single);
    list->AddItem(variant % 2 == 0 ? "One" : "Alpha");
    list->AddItem(variant % 2 == 0 ? "Two" : "Beta");
    list->AddItem(variant % 2 == 0 ? "Three" : "Gamma");
    list->SetSelectedIndex(0);

    auto input = std::make_shared<TextBox>("Type something...");
    input->SetWidth(360.0f);

    return Column(12).Children({
        std::make_shared<TextBlock>(title),
        std::make_shared<TextBlock>(body),
        chk,
        toggle,
        combo,
        list,
        input
    }).Build();
}

} // namespace

ShowcasePage BuildNavigationViewPage(const ShowcaseContext& ctx) {
    auto nav = std::make_shared<NavigationView>();
    nav->SetPaneTitle("CUI");
    nav->SetHeader("Home");
    nav->SetAlwaysShowHeader(true);
    nav->SetPaneDisplayMode(NavigationViewPaneDisplayMode::Auto);
    nav->SetIsSettingsVisible(true);

    auto pageHome = MakePage("Home", "WinUI 3 NavigationView: PaneDisplayMode / DisplayMode / IsPaneOpen 三者分离。", 0);
    auto pageApps = MakePage("Apps", "MenuItems + FooterMenuItems + SettingsItem 共用单一选中模型。", 1);
    auto pageDocs = MakePage("Documents", "支持 Header / Separator / 一层层级 MenuItems。", 2);
    auto pageMusic = MakePage("Music", "LeftCompact / LeftMinimal 下 IsPaneOpen 控制 overlay/inline 行为。", 3);
    auto pageSettings = MakePage("Settings", "内置 SettingsItem；ItemInvoked → SelectionChanged。", 4);

    nav->SetContent(pageHome);

    // MenuItems
    auto home = std::make_shared<NavigationViewItem>("Home", "🏠");
    home->SetTag("home");
    nav->AddMenuItem(home);

    nav->AddMenuItem(std::make_shared<NavigationViewItemHeader>("Library"));

    auto apps = std::make_shared<NavigationViewItem>("Apps", "⚡");
    apps->SetTag("apps");
    nav->AddMenuItem(apps);

    auto docs = std::make_shared<NavigationViewItem>("Documents", "📄");
    docs->SetTag("docs");
    // Hierarchy: parent does not select; expands children.
    docs->SetSelectsOnInvoked(false);
    auto docsAll = std::make_shared<NavigationViewItem>("All files", "📁");
    docsAll->SetTag("docs-all");
    auto docsRecent = std::make_shared<NavigationViewItem>("Recent", "🕒");
    docsRecent->SetTag("docs-recent");
    docs->AddMenuItem(docsAll);
    docs->AddMenuItem(docsRecent);
    nav->AddMenuItem(docs);

    nav->AddMenuItem(std::make_shared<NavigationViewItemSeparator>());

    auto music = std::make_shared<NavigationViewItem>("Music", "🎵");
    music->SetTag("music");
    nav->AddMenuItem(music);

    // Footer
    auto account = std::make_shared<NavigationViewItem>("Account", "👤");
    account->SetTag("account");
    nav->AddFooterMenuItem(account);

    // AutoSuggest slot
    auto search = std::make_shared<TextBox>();
    search->SetPlaceholder("Search");
    search->SetHeight(32.0f);
    nav->SetAutoSuggestBox(search);

    nav->SetSelectedItem(home.get());

    nav->OnItemInvoked().Connect([nav, pageHome, pageApps, pageDocs, pageMusic, pageSettings,
                                  docsAll, docsRecent](NavigationView*, const NavigationViewItemInvokedEventArgs& args) {
        if (!args.InvokedItem) {
            return;
        }
        if (args.IsSettingsInvoked) {
            nav->SetHeader("Settings");
            nav->SetContent(pageSettings);
            return;
        }
        const std::string& tag = args.InvokedItem->GetTag();
        if (tag == "home") {
            nav->SetHeader("Home");
            nav->SetContent(pageHome);
        } else if (tag == "apps") {
            nav->SetHeader("Apps");
            nav->SetContent(pageApps);
        } else if (tag == "docs" || tag == "docs-all" || tag == "docs-recent") {
            nav->SetHeader("Documents");
            nav->SetContent(pageDocs);
        } else if (tag == "music") {
            nav->SetHeader("Music");
            nav->SetContent(pageMusic);
        } else if (tag == "account") {
            nav->SetHeader("Account");
            nav->SetContent(MakePage("Account", "FooterMenuItems 与 MenuItems 共享选中。", 5));
        }
        (void)docsAll;
        (void)docsRecent;
    });

    // PaneDisplayMode switcher
    auto modeBox = std::make_shared<ComboBox>();
    modeBox->AddItem("Auto");
    modeBox->AddItem("Left");
    modeBox->AddItem("LeftCompact");
    modeBox->AddItem("LeftMinimal");
    modeBox->AddItem("Top");
    modeBox->SetSelectedIndex(0);
    modeBox->OnSelectionChanged().Connect([nav](ComboBox* box, int index, const std::string&) {
        static const NavigationViewPaneDisplayMode kModes[] = {
            NavigationViewPaneDisplayMode::Auto,
            NavigationViewPaneDisplayMode::Left,
            NavigationViewPaneDisplayMode::LeftCompact,
            NavigationViewPaneDisplayMode::LeftMinimal,
            NavigationViewPaneDisplayMode::Top
        };
        if (index >= 0 && index < 5) {
            nav->SetPaneDisplayMode(kModes[index]);
        }
        (void)box;
    });

    auto chkHeader = std::make_shared<CheckBox>();
    chkHeader->SetText("AlwaysShowHeader");
    chkHeader->SetState(CheckState::Checked);
    chkHeader->OnCheckStateChanged().Connect([nav](CheckBox*, CheckState state) {
        nav->SetAlwaysShowHeader(state == CheckState::Checked);
    });

    auto btnToggle = std::make_shared<Button>("Toggle Pane");
    btnToggle->OnClick().Connect([nav](UIElement*) {
        nav->TogglePane();
    });

    auto modeLabel = std::make_shared<TextBlock>("PaneDisplayMode");
    nav->SetWidth(860.0f);
    nav->SetHeight(420.0f);

    auto demo = Column(12).Children({
        Row(12).Children({ modeLabel, modeBox, btnToggle, chkHeader }).Build(),
        nav
    }).Build();

    return { "NavigationView 导航", CreatePage(
        "WinUI 3 NavigationView",
        "PaneDisplayMode / DisplayMode / IsPaneOpen 分离；MenuItems · Footer · Settings · Header/Separator · 层级 · Top。",
        demo) };
}
