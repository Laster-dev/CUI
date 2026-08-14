#include "chrome/GalleryShell.h"
#include "chrome/HomePage.h"
#include "chrome/ConventionsPage.h"
#include "chrome/SettingsPage.h"
#include "catalog/Catalog.h"
#include "GalleryHost.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/AutoSuggestBox.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/NavigationViewItem.h"
#include "framework/controls/ToggleButton.h"
#include "framework/controls/ToastCenter.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/window/Window.h"

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {
    namespace {

        constexpr const char* kHomeTag = "home";
        constexpr const char* kConventionsTag = "conventions";
        constexpr const char* kSettingsTag = "settings";
        constexpr size_t kMaxCachedPages = 12;

        struct PageCache {
            std::unordered_map<std::string, std::shared_ptr<UIElement>> content;
            std::list<std::string> lru;

            std::shared_ptr<UIElement> Resolve(const std::string& tag) {
                if (tag == kHomeTag) {
                    return BuildHomePage();
                }
                if (tag == kSettingsTag) {
                    return BuildSettingsPage();
                }
                if (tag == kConventionsTag) {
                    return BuildConventionsPage();
                }

                auto cached = content.find(tag);
                if (cached != content.end()) {
                    lru.remove(tag);
                    lru.push_front(tag);
                    return cached->second;
                }

                const Entry* entry = FindByTag(tag);
                if (!entry || !entry->build) {
                    return nullptr;
                }
                auto page = entry->build();
                content.emplace(tag, page);
                lru.push_front(tag);
                while (lru.size() > kMaxCachedPages) {
                    const std::string evict = lru.back();
                    lru.pop_back();
                    if (evict != tag) {
                        content.erase(evict);
                    }
                }
                return page;
            }

            bool Contains(const std::string& tag) const {
                return content.find(tag) != content.end();
            }
        };

        std::shared_ptr<NavigationView> BuildNavigation() {
            auto nav = std::make_shared<NavigationView>();
            nav->SetHeader(std::string());
            nav->SetPaneTitle("CUI Gallery");
			//nav->SetIcon("Assets/Logo.png");
            nav->SetPaneDisplayMode(NavigationViewPaneDisplayMode::Auto);
            nav->SetIsSettingsVisible(true);
            if (auto* settings = nav->SettingsItem()) {
                settings->SetContent("设置");
            }
            nav->SetIsPaneOpen(true);
            nav->SetFlexGrow(1.0f);
            nav->SetAlign(Alignment::Stretch);

            auto homeItem = std::make_shared<NavigationViewItem>("主页");
            homeItem->SetTag(kHomeTag);
            nav->AddMenuItem(homeItem);

            auto conventionsItem = std::make_shared<NavigationViewItem>("全局约定");
            conventionsItem->SetTag(kConventionsTag);
            nav->AddMenuItem(conventionsItem);

            for (Category category : CategoryOrder()) {
                auto items = EntriesIn(category);
                if (items.empty()) {
                    continue;
                }
                auto folder = std::make_shared<NavigationViewItem>(CategoryDisplayName(category));
                folder->SetSelectsOnInvoked(false);
                for (const Entry* entry : items) {
                    auto child = std::make_shared<NavigationViewItem>(entry->title);
                    child->SetTag(entry->tag);
                    folder->AddMenuItem(child);
                }
                nav->AddMenuItem(folder);
            }

            auto search = std::make_shared<AutoSuggestBox>();
            search->SetPlaceholder("搜索控件");
            search->SetMaxVisibleSuggestions(10);
            {
                std::vector<std::string> titles;
                for (const auto& e : Entries()) {
                    titles.push_back(e.title);
                }
                search->SetSuggestionItems(titles);
                search->SetSuggestionProvider([](const std::string& query) {
                    return SearchTitles(query);
                    });
            }
            nav->SetAutoSuggestBox(search);

            auto cache = std::make_shared<PageCache>();

            auto navigate = [nav, cache](const std::string& tag) {
                if (tag.empty()) {
                    return;
                }
                nav->NavigateTo(tag);
                if (tag == kSettingsTag || tag == kConventionsTag) {
                    nav->SetContent(cache->Resolve(tag));
                    return;
                }
                if (cache->Contains(tag) || tag == kHomeTag) {
                    if (auto page = cache->Resolve(tag)) {
                        nav->SetContent(page);
                    }
                    return;
                }
                nav->SetContentFactory([cache, tag]() {
                    return cache->Resolve(tag);
                });
            };

            Host::Instance().SetNavigator(navigate);
            search->OnSuggestionChosen().Connect([search](AutoSuggestBox*, const std::string& title) {
                if (const Entry* entry = FindByTitle(title)) {
                    Host::Instance().Navigate(entry->tag);
                    search->SetText("");
                }
                });
            search->OnQuerySubmitted().Connect([search](AutoSuggestBox*, const std::string& query) {
                if (const Entry* exact = FindByTitle(query)) {
                    Host::Instance().Navigate(exact->tag);
                    search->SetText("");
                    return;
                }
                auto matches = SearchTitles(query);
                if (!matches.empty()) {
                    if (const Entry* entry = FindByTitle(matches.front())) {
                        Host::Instance().Navigate(entry->tag);
                    }
                    search->SetText("");
                }
                });

            nav->OnItemInvoked().Connect([navigate](NavigationView*, const NavigationViewItemInvokedEventArgs& args) {
                if (args.IsSettingsInvoked) {
                    navigate(kSettingsTag);
                    return;
                }
                if (!args.InvokedItem) {
                    return;
                }
                const std::string& tag = args.InvokedItem->GetTag();
                if (tag.empty()) {
                    return;
                }
                navigate(tag);
                });

            nav->SetSelectedItem(homeItem.get());
            nav->SetContent(cache->Resolve(kHomeTag));
            return nav;
        }

    } // namespace

    std::shared_ptr<UIElement> BuildGalleryRoot() {
        auto titleBar = std::make_shared<WindowTitleBar>();
        titleBar->SetTitle("CUI Gallery");
		//构建主题切换控件
        auto ThemeModeRange = std::make_shared<SegmentedControl>();
        ThemeModeRange->SetWidth(120.0f);
		ThemeModeRange->SetMargin(Thickness(2, 2, 10, 2));
        ThemeModeRange->AddItem("Dark");
        ThemeModeRange->AddItem("Light");
        ThemeModeRange->OnSelectionChanged().Connect([](SegmentedControl*, int, const std::string& item) {
            if (auto* window = Window::Current()) {
                window->SetThemeMode(item == "Dark" ? ThemeMode::Dark : ThemeMode::Light);
            }
        });
		//构建动画|低性能模式切换控件
		auto AnimationModeRange = std::make_shared<SegmentedControl>();
		AnimationModeRange->SetWidth(120.0f);
		AnimationModeRange->SetMargin(Thickness(2, 2, 10, 2));
		AnimationModeRange->AddItem("动画");
		AnimationModeRange->AddItem("低性能");
        AnimationModeRange->OnSelectionChanged().Connect([](SegmentedControl*, int, const std::string& item) {
           UIElement::SetAnimationsEnabled(item == "动画");
		});

		//把两个控件合并为一个水平布局，然后添加
		auto rightContent = std::make_shared<StackPanel>();
		rightContent->SetOrientation(Orientation::Horizontal);
		rightContent->AddChild(ThemeModeRange);
		rightContent->AddChild(AnimationModeRange);
		//设置右侧内容
		titleBar->SetRightContent(rightContent);


       
        auto fileMenu = titleBar->GetMenuBar().AddMenu("文件");
        fileMenu->AddItem("主页", [] {
            Host::Instance().Navigate("home");
            });
        fileMenu->AddItem("设置", [] {
            Host::Instance().Navigate("settings");
            });
        fileMenu->AddSeparator();
        fileMenu->AddItem("退出", "Alt+F4", [] {
            if (auto* win = Window::Current()) {
                if (HWND hwnd = win->GetHWND()) {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                }
            }
            });

        auto nav = BuildNavigation();
        auto toastCenter = std::make_shared<ToastCenter>();
        toastCenter->SetId("toastCenter");

        auto root = Column(0).Children({ titleBar, nav, toastCenter }).Build();
        root->SetBackgroundToken(ThemeTokenId::WindowBackground);
        return root;
    }

} // namespace Gallery
