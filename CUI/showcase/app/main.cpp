#include "ShowcaseContext.h"
#include "Streaming.h"
#include "pages/PageRegistry.h"
#include "ShowcaseHelpers.h"
#include "framework/window/Window.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/ToastCenter.h"
#include "chrome/VSCodeControls.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/NavigationViewItem.h"
#include "framework/controls/AutoSuggestBox.h"
#include "framework/controls/ProgressBarDiag.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ComboBox.h"
#include <shellscalingapi.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <list>

using namespace CUI;
using namespace CUI::DSL;

static std::shared_ptr<Toast> BuildToastTemplate() {
    auto toast = std::make_shared<Toast>();
    toast->SetTitle("XML Toast");
    toast->SetMessage("这是从旧展示模板回填的声明式通知");
    toast->SetCorner(ToastCorner::BottomRight);
    toast->SetDurationMs(2800);
    toast->SetAutoClose(true);
    toast->SetCloseable(true);
    toast->SetWidth(320.0f);
    toast->SetSpacing(8.0f);
    return toast;
}

class GalleryNavigation : public StatelessWidget {
public:
    explicit GalleryNavigation(const ShowcaseContext& ctx, BuildContext buildContext = {})
        : StatelessWidget(buildContext), m_ctx(ctx) {}

    std::shared_ptr<UIElement> build(BuildContext& context) override {
        auto win = context.window ? context.window : m_ctx.windowRef;

        struct SampleEntry {
            std::string tag;
            std::string category;
            std::string label;
            std::function<ShowcasePage()> factory;
        };

        std::vector<SampleEntry> samples;
        auto add = [&](const std::string& tag, const std::string& category, const std::string& label,
                       std::function<ShowcasePage()> factory) {
            samples.push_back(SampleEntry{ tag, category, label, std::move(factory) });
        };

        // Factories only — pages build on first navigate (browser/Flutter route style).
        add("button", "基础 Controls", "Button", [ctx = m_ctx] { return BuildButtonPage(ctx); });
        add("textblock", "基础 Controls", "TextBlock", [ctx = m_ctx] { return BuildTextBlockPage(ctx); });
        add("textbox", "基础 Controls", "TextBox", [ctx = m_ctx] { return BuildTextBoxPage(ctx); });
        add("passwordbox", "基础 Controls", "PasswordBox", [ctx = m_ctx] { return BuildPasswordBoxPage(ctx); });
        add("checkbox", "基础 Controls", "CheckBox", [ctx = m_ctx] { return BuildCheckBoxPage(ctx); });
        add("radiobutton", "基础 Controls", "RadioButton", [ctx = m_ctx] { return BuildRadioButtonPage(ctx); });
        add("toggleswitch", "基础 Controls", "ToggleSwitch", [ctx = m_ctx] { return BuildToggleSwitchPage(ctx); });
        add("combobox", "基础 Controls", "ComboBox", [ctx = m_ctx] { return BuildComboBoxPage(ctx); });
        add("autosuggest", "基础 Controls", "AutoSuggestBox", [ctx = m_ctx] { return BuildAutoSuggestPage(ctx); });
        add("statusbar", "基础 Controls", "StatusBar", [ctx = m_ctx] { return BuildStatusBarPage(ctx); });

        add("slider", "值/进度", "Slider", [ctx = m_ctx] { return BuildSliderPage(ctx); });
        add("progressbar", "值/进度", "ProgressBar", [ctx = m_ctx] { return BuildProgressBarPage(ctx); });
        add("filepicker", "值/进度", "FilePicker", [ctx = m_ctx] { return BuildFilePickerPage(ctx); });
        add("numberbox", "值/进度", "NumberBox", [ctx = m_ctx] { return BuildNumberBoxPage(ctx); });
        add("datepicker", "值/进度", "DatePicker", [ctx = m_ctx] { return BuildDatePickerPage(ctx); });
        add("timepicker", "值/进度", "TimePicker", [ctx = m_ctx] { return BuildTimePickerPage(ctx); });
        add("colorpicker", "值/进度", "ColorPicker", [ctx = m_ctx] { return BuildColorPickerPage(ctx); });

        add("breadcrumb", "导航与数据", "Breadcrumb", [ctx = m_ctx] { return BuildBreadcrumbPage(ctx); });
        add("paging", "导航与数据", "Paging", [ctx = m_ctx] { return BuildPagingPage(ctx); });
        add("treeview", "导航与数据", "TreeView", [ctx = m_ctx] { return BuildTreeViewPage(ctx); });
        add("hyperlink", "导航与数据", "Hyperlink", [ctx = m_ctx] { return BuildHyperlinkPage(ctx); });
        add("listbox", "列表", "ListBox", [ctx = m_ctx] { return BuildListBoxPage(ctx); });
        add("listview", "列表", "ListView", [ctx = m_ctx] { return BuildListViewPage(ctx); });

        add("splitter", "布局", "Splitter", [ctx = m_ctx] { return BuildSplitterPage(ctx); });
        add("collapse", "布局", "Expander", [ctx = m_ctx] { return BuildCollapsePage(ctx); });
        add("grid", "布局", "Grid", [ctx = m_ctx] { return BuildGridPage(ctx); });
        add("wrap", "布局", "Wrap", [ctx = m_ctx] { return BuildWrapPage(ctx); });
        add("dock", "布局", "Dock", [ctx = m_ctx] { return BuildDockPage(ctx); });
        add("docking", "布局", "Docking", [ctx = m_ctx] { return BuildDockingPage(ctx); });
        add("uniformgrid", "布局", "UniformGrid", [ctx = m_ctx] { return BuildUniformPage(ctx); });
        add("stackpanel", "布局", "StackPanel", [ctx = m_ctx] { return BuildStackPanelPage(ctx); });
        add("scrollviewer", "布局", "ScrollViewer", [ctx = m_ctx] { return BuildScrollViewerPage(ctx); });

        add("flyout", "交互/浮层", "Flyout", [ctx = m_ctx] { return BuildFlyoutPage(ctx); });
        add("dialog", "交互/浮层", "Dialog", [ctx = m_ctx] { return BuildDialogPage(ctx); });
        add("toast", "交互/浮层", "Toast", [ctx = m_ctx] { return BuildToastPage(ctx); });
        add("stream", "终端/媒体", "Stream", [ctx = m_ctx] { return BuildStreamPage(ctx); });
        add("terminal", "终端/媒体", "Terminal", [ctx = m_ctx] { return BuildTerminalPage(ctx); });

        add("navigationview", "交互/导航", "NavigationView", [ctx = m_ctx] { return BuildNavigationViewPage(ctx); });
        add("tabview", "交互/导航", "TabView", [ctx = m_ctx] { return BuildTabViewPage(ctx); });
        add("canvas", "画布与渲染", "Canvas", [ctx = m_ctx] { return BuildCanvasPage(ctx); });

        // --- Settings content ---
        std::shared_ptr<UIElement> settingsContent;
        if (win) {
            auto title = CreateShowcaseHeader("Settings", "Theme 配置");

            auto btnDark = std::make_shared<Button>("Dark Theme");
            auto btnLight = std::make_shared<Button>("Light Theme");
            btnDark->OnClick().Connect([win](UIElement*) {
                win->SetThemeMode(ThemeMode::Dark);
            });
            btnLight->OnClick().Connect([win](UIElement*) {
                win->SetThemeMode(ThemeMode::Light);
            });

            auto demo = Column(12).Children({
                Row(12).Children({ btnDark, btnLight }).Build(),
            }).Build();

            settingsContent = Column(16).Padding(20).Children({ title, demo }).Build();
        } else {
            settingsContent = CreateShowcaseText("Settings", 18.0f, "textPrimary", true);
        }

        auto nav = std::make_shared<NavigationView>();
        nav->SetHeader(std::string());
        nav->SetPaneTitle("CUI Control Gallery");
        nav->SetPaneDisplayMode(NavigationViewPaneDisplayMode::Auto);
        nav->SetIsBackButtonVisible(NavigationViewBackButtonVisible::Collapsed);
        nav->SetIsSettingsVisible(true);
        nav->SetIsPaneOpen(true);

        // Sidebar control search (AutoSuggestBox slot on NavigationView).
        auto search = std::make_shared<AutoSuggestBox>();
        search->SetPlaceholder("搜索控件…");
        search->SetHeight(32.0f);
        search->SetFontSize(12.0f);
        search->SetMaxVisibleSuggestions(10);
        {
            std::vector<std::string> labels;
            labels.reserve(samples.size());
            for (const auto& s : samples) {
                labels.push_back(s.label);
            }
            search->SetSuggestionItems(labels);
            // Match label / tag / category (case-insensitive substring).
            search->SetSuggestionProvider([samples](const std::string& query) {
                auto lower = [](std::string s) {
                    for (char& c : s) {
                        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
                    }
                    return s;
                };
                const std::string q = lower(query);
                std::vector<std::string> out;
                if (q.empty()) {
                    return out;
                }
                for (const auto& s : samples) {
                    if (lower(s.label).find(q) != std::string::npos
                        || lower(s.tag).find(q) != std::string::npos
                        || lower(s.category).find(q) != std::string::npos) {
                        out.push_back(s.label);
                    }
                }
                return out;
            });
        }
        nav->SetAutoSuggestBox(search);

        struct PageCache {
            std::unordered_map<std::string, std::shared_ptr<UIElement>> content;
            std::list<std::string> lru;
            std::vector<SampleEntry> samples;

            std::shared_ptr<UIElement> Resolve(const std::string& tag) {
            constexpr size_t kMaxCachedPages = 12;
                auto cached = content.find(tag);
                if (cached != content.end()) {
                    lru.remove(tag);
                    lru.push_front(tag);
                    return cached->second;
                }
                for (const auto& s : samples) {
                    if (s.tag != tag || !s.factory) {
                        continue;
                    }
                    ShowcasePage page = s.factory();
                    content.emplace(tag, page.content);
                    lru.push_front(tag);
                    while (lru.size() > kMaxCachedPages) {
                        const std::string evict = lru.back();
                        lru.pop_back();
                        if (evict != tag) {
                            content.erase(evict);
                        }
                    }
                    return page.content;
                }
                return nullptr;
            }

            bool Contains(const std::string& tag) const {
                return content.find(tag) != content.end();
            }
        };
        auto pageCache = std::make_shared<PageCache>();
        pageCache->samples = samples;

        std::string initialTag = samples.empty() ? std::string() : samples.front().tag;
        if (!samples.empty()) {
            for (const auto& s : samples) {
                if (s.tag == "button") {
                    initialTag = "button";
                    break;
                }
            }
        }

        struct CategoryBucket {
            std::string name;
            std::vector<const SampleEntry*> items;
        };
        std::unordered_map<std::string, CategoryBucket> buckets;
        for (const auto& s : samples) {
            if (!buckets.count(s.category)) {
                buckets.emplace(s.category, CategoryBucket{ s.category, {} });
            }
            buckets[s.category].items.push_back(&s);
        }

        std::shared_ptr<NavigationViewItem> firstItem;
        auto findFirstChild = [&](const std::string& tag) -> std::shared_ptr<NavigationViewItem> {
            for (auto& kv : buckets) {
                for (const auto* item : kv.second.items) {
                    if (item->tag == tag) {
                        return std::shared_ptr<NavigationViewItem>();
                    }
                }
            }
            return std::shared_ptr<NavigationViewItem>();
        };
        (void)findFirstChild;

        // Deterministic category order.
        const std::vector<std::string> orderedCats = {
            "基础 Controls",
            "值/进度",
            "导航与数据",
            "列表",
            "布局",
            "交互/浮层",
            "交互/导航",
            "画布与渲染",
            "终端/媒体"
        };

        // Build NavigationView items.
        for (const auto& catName : orderedCats) {
            auto it = buckets.find(catName);
            if (it == buckets.end()) continue;
            if (it->second.items.empty()) continue;

            auto catItem = std::make_shared<NavigationViewItem>(catName);
            catItem->SetSelectsOnInvoked(false);

            for (const auto* s : it->second.items) {
                auto child = std::make_shared<NavigationViewItem>(s->label);
                child->SetTag(s->tag);
                catItem->AddMenuItem(child);

                if (!firstItem && !initialTag.empty() && s->tag == initialTag) {
                    firstItem = child;
                }
            }
            nav->AddMenuItem(catItem);
        }

        // Fallback if initialTag wasn't found.
        if (!firstItem && !samples.empty()) {
            auto fallbackTag = samples.front().tag;
            for (const auto& catName : orderedCats) {
                auto it = buckets.find(catName);
                if (it == buckets.end()) continue;
                for (const auto* s : it->second.items) {
                    if (s->tag == fallbackTag) {
                        // We need to find the corresponding NavigationViewItem pointer,
                        // but we didn't store it. Use SetSelectedItem by tag.
                        nav->SelectByTag(s->tag);
                        firstItem = nullptr;
                        break;
                    }
                }
                if (firstItem || nav->GetSelectedItem()) break;
            }
        }

        if (!initialTag.empty()) {
            if (auto content = pageCache->Resolve(initialTag)) {
                nav->SetContent(content);
            }
        } else if (!samples.empty()) {
            initialTag = samples.front().tag;
            if (auto content = pageCache->Resolve(initialTag)) {
                nav->SetContent(content);
            }
        }

        // Ensure indicator starts in sync.
        if (!firstItem && !initialTag.empty()) {
            nav->SelectByTag(initialTag);
        } else if (firstItem) {
            nav->SetSelectedItem(firstItem.get());
        }

        // Navigate to a sample by tag (shared by sidebar click + search).
        auto navigateToTag = [nav, win, streamImg = m_ctx.streamImage, pageCache](const std::string& tag) {
            if (tag.empty()) {
                return;
            }
            nav->SelectByTag(tag);
            CUI::ProgressBarDiag::Log("[PB] gallery navigate tag=%s cached=%d",
                tag.c_str(), pageCache->Contains(tag) ? 1 : 0);
            if (pageCache->Contains(tag)) {
                if (auto content = pageCache->Resolve(tag)) {
                    nav->SetContent(content);
                }
            } else {
                nav->SetContentFactory([pageCache, tag]() {
                    CUI::ProgressBarDiag::Log("[PB] gallery factory Resolve(%s)", tag.c_str());
                    return pageCache->Resolve(tag);
                });
            }
            if (win) {
                if (tag == "stream") StartStreamingThread(win, streamImg);
                else StopStreamingThread();
            }
        };

        auto labelToTag = std::make_shared<std::unordered_map<std::string, std::string>>();
        for (const auto& s : samples) {
            (*labelToTag)[s.label] = s.tag;
        }

        auto goFromSearchLabel = [navigateToTag, labelToTag, search](const std::string& label) {
            auto it = labelToTag->find(label);
            if (it == labelToTag->end()) {
                return;
            }
            navigateToTag(it->second);
            search->SetText("");
        };

        search->OnSuggestionChosen().Connect(
            [goFromSearchLabel](AutoSuggestBox*, const std::string& label) {
                goFromSearchLabel(label);
            });
        search->OnQuerySubmitted().Connect(
            [goFromSearchLabel, labelToTag, search, samples](AutoSuggestBox*, const std::string& query) {
                // Exact label hit, else first fuzzy match on label/tag.
                if (labelToTag->count(query)) {
                    goFromSearchLabel(query);
                    return;
                }
                auto lower = [](std::string s) {
                    for (char& c : s) {
                        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
                    }
                    return s;
                };
                const std::string q = lower(query);
                if (q.empty()) {
                    return;
                }
                for (const auto& s : samples) {
                    if (lower(s.label).find(q) != std::string::npos
                        || lower(s.tag).find(q) != std::string::npos) {
                        goFromSearchLabel(s.label);
                        return;
                    }
                }
            });

        // Switch content on invocation.
        nav->OnItemInvoked().Connect([nav, settingsContent, win, navigateToTag](
                                         NavigationView*, const NavigationViewItemInvokedEventArgs& args) mutable {
            if (args.IsSettingsInvoked) {
                if (win) StopStreamingThread();
                nav->SetContent(settingsContent);
                return;
            }
            if (!args.InvokedItem) return;
            navigateToTag(args.InvokedItem->GetTag());
        });

        return nav;
    }

private:
    ShowcaseContext m_ctx;
};

class GalleryShell : public StatelessWidget {
public:
    explicit GalleryShell(const ShowcaseContext& ctx, BuildContext buildContext = {})
        : StatelessWidget(buildContext), m_ctx(ctx) {}

    std::shared_ptr<UIElement> build(BuildContext& context) override {
        auto titleBar = std::make_shared<TitleBar>();
        titleBar->SetTitle("CUI Control Gallery | Flutter 风格 C++ 声明式 Widget Showcase");

        auto toastCenter = std::make_shared<ToastCenter>();
        toastCenter->SetId("toastCenter");

        auto effectiveContext = m_ctx;
        if (context.window) {
            effectiveContext.windowRef = context.window;
        }

        if (effectiveContext.windowRef) {
            titleBar->OnToggleLowPerformance().Connect([win = effectiveContext.windowRef](TitleBar*) {
                win->SetLowPerformanceMode(!win->IsLowPerformanceMode());
            });
            titleBar->OnToggleTheme().Connect([win = effectiveContext.windowRef](TitleBar*) {
                win->SetThemeMode(win->GetThemeMode() == ThemeMode::Dark ? ThemeMode::Light : ThemeMode::Dark);
            });
        }

        // Root chrome/content must not use the default Column gap. ToastCenter is
        // a zero-size overlay host; if it participates in a gapped Column, the
        // flex layout still reserves an 8px gap before it and the main content
        // stops short of the bottom edge on the first layout.
        return Column(0).Children({
            titleBar,
            Expanded(GalleryNavigation(effectiveContext, context).Build()),
            toastCenter,
            std::make_shared<GalleryPerfStatusBar>()
        }).Build();
    }

private:
    ShowcaseContext m_ctx;
};

static std::shared_ptr<UIElement> BuildRoot(const ShowcaseContext& ctx) {
    BuildContext buildContext;
    buildContext.window = ctx.windowRef;
    return GalleryShell(ctx, buildContext).Build();
}

int main() {
    // This app draws its own pixels and text via Direct2D, so it must opt into
    // per-monitor DPI awareness before creating any HWND. Otherwise Windows
    // treats the process as DPI-unaware, bitmap-scales the first frame, and the
    // whole window looks soft until later size changes force a sharper redraw.
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware();
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return -1;

    std::cout << "========================================================\n";
    std::cout << "   CUI Control Gallery - 100% Pure C++ Declarative UI   \n";
    std::cout << "========================================================\n";

    {
        Window window;
        if (!window.Create("CUI Control Gallery - 全套旧展示页纯 C++ 声明式 Widget 回填版", 1280, 780, false)) {
            CoUninitialize();
            return -1;
        }

        ShowcaseContext ctx;
        ctx.windowRef = &window;
        ctx.streamImage = CreateStreamImage();
        ctx.toastTemplate = BuildToastTemplate();

        window.SetRootElement(BuildRoot(ctx));
        window.Show();
        window.RunMessageLoop();

        StopStreamingThread();
    }

    CoUninitialize();
    return 0;
}
