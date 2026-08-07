#include "ShowcaseContext.h"
#include "Streaming.h"
#include "pages/PageRegistry.h"
#include "ShowcaseHelpers.h"
#include "framework/window/Window.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/ToastCenter.h"
#include "framework/controls/VSCodeControls.h"
#include "framework/controls/NavigationView.h"
#include "framework/controls/NavigationViewItem.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ComboBox.h"
#include <shellscalingapi.h>
#include <iostream>
#include <vector>
#include <unordered_map>

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
            std::shared_ptr<UIElement> content;
            std::string label;
        };

        std::vector<SampleEntry> samples;
        auto add = [&](const std::string& tag, const std::string& category, const ShowcasePage& page) {
            samples.push_back(SampleEntry{ tag, category, page.content, page.tabTitle });
        };

        // --- Build samples (grouped like WinUI3 Gallery) ---
        add("button", "基础 Controls", BuildButtonPage(m_ctx));
        add("textblock", "基础 Controls", BuildTextBlockPage(m_ctx));
        add("textbox", "基础 Controls", BuildTextBoxPage(m_ctx));
        add("passwordbox", "基础 Controls", BuildPasswordBoxPage(m_ctx));
        add("checkbox", "基础 Controls", BuildCheckBoxPage(m_ctx));
        add("radiobutton", "基础 Controls", BuildRadioButtonPage(m_ctx));
        add("toggleswitch", "基础 Controls", BuildToggleSwitchPage(m_ctx));
        add("combobox", "基础 Controls", BuildComboBoxPage(m_ctx));

        add("slider", "值/进度", BuildSliderPage(m_ctx));
        add("progressbar", "值/进度", BuildProgressBarPage(m_ctx));
        add("numberbox", "值/进度", BuildNumberBoxPage(m_ctx));
        add("datepicker", "值/进度", BuildDatePickerPage(m_ctx));
        add("timepicker", "值/进度", BuildTimePickerPage(m_ctx));
        add("colorpicker", "值/进度", BuildColorPickerPage(m_ctx));

        add("breadcrumb", "导航与数据", BuildBreadcrumbPage(m_ctx));
        add("paging", "导航与数据", BuildPagingPage(m_ctx));
        add("treeview", "导航与数据", BuildTreeViewPage(m_ctx));
        add("hyperlink", "导航与数据", BuildHyperlinkPage(m_ctx));
        add("listbox", "列表", BuildListBoxPage(m_ctx));
        add("listview", "列表", BuildListViewPage(m_ctx));

        add("splitter", "布局", BuildSplitterPage(m_ctx));
        add("collapse", "布局", BuildCollapsePage(m_ctx));
        add("grid", "布局", BuildGridPage(m_ctx));
        add("wrap", "布局", BuildWrapPage(m_ctx));
        add("dock", "布局", BuildDockPage(m_ctx));
        add("uniformgrid", "布局", BuildUniformPage(m_ctx));
        add("stackpanel", "布局", BuildStackPanelPage(m_ctx));
        add("scrollviewer", "布局", BuildScrollViewerPage(m_ctx));

        add("flyout", "交互/浮层", BuildFlyoutPage(m_ctx));
        add("dialog", "交互/浮层", BuildDialogPage(m_ctx));
        add("toast", "交互/浮层", BuildToastPage(m_ctx));
        add("stream", "终端/媒体", BuildStreamPage(m_ctx));
        add("terminal", "终端/媒体", BuildTerminalPage(m_ctx));

        add("navigationview", "交互/导航", BuildNavigationViewPage(m_ctx));
        add("tabview", "交互/导航", BuildTabViewPage(m_ctx));
        add("canvas", "画布与渲染", BuildCanvasPage(m_ctx));

        // --- Settings content (WinUI Gallery: theme/backdrop) ---
        std::shared_ptr<UIElement> settingsContent;
        if (win) {
            auto title = CreateShowcaseHeader("Settings", "Theme / Backdrop 配置");
            auto cbBackdrop = std::make_shared<ComboBox>();
            cbBackdrop->AddItem("云母");
            cbBackdrop->AddItem("沉浸云母");
            cbBackdrop->AddItem("亚克力");
            cbBackdrop->AddItem("无材质");
            int defaultIndex = 3;
            switch (win->GetBackdropType()) {
            case BackdropType::Mica: defaultIndex = 0; break;
            case BackdropType::MicaAlt: defaultIndex = 1; break;
            case BackdropType::Acrylic: defaultIndex = 2; break;
            case BackdropType::None: defaultIndex = 3; break;
            }
            cbBackdrop->SetSelectedIndex(defaultIndex);

            cbBackdrop->OnSelectionChanged().Connect([win](ComboBox*, int index, const std::string&) {
                switch (index) {
                case 0: win->SetBackdropType(BackdropType::Mica); break;
                case 1: win->SetBackdropType(BackdropType::MicaAlt); break;
                case 2: win->SetBackdropType(BackdropType::Acrylic); break;
                case 3: win->SetBackdropType(BackdropType::None); break;
                }
            });

            auto btnDark = std::make_shared<Button>("Dark Theme");
            auto btnLight = std::make_shared<Button>("Light Theme");
            btnDark->OnClick().Connect([win](UIElement*) {
                win->SetThemeMode(ThemeMode::Dark);
            });
            btnLight->OnClick().Connect([win](UIElement*) {
                win->SetThemeMode(ThemeMode::Light);
            });

            auto demo = Column(12).Children({
                Row(12).Children({
                    std::make_shared<TextBlock>("Backdrop:"), cbBackdrop
                }).Build(),
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

        std::unordered_map<std::string, std::shared_ptr<UIElement>> contentByTag;
        contentByTag.reserve(samples.size());
        for (const auto& s : samples) {
            contentByTag.emplace(s.tag, s.content);
        }

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

        if (!initialTag.empty() && contentByTag.count(initialTag)) {
            nav->SetContent(contentByTag[initialTag]);
        } else if (!samples.empty()) {
            nav->SetContent(samples.front().content);
            initialTag = samples.front().tag;
        }

        // Ensure indicator starts in sync.
        if (!firstItem && !initialTag.empty()) {
            nav->SelectByTag(initialTag);
        } else if (firstItem) {
            nav->SetSelectedItem(firstItem.get());
        }

        // Switch content on invocation.
        nav->OnItemInvoked().Connect([nav, settingsContent, win, streamImg = m_ctx.streamImage,
                                       contentByTag](NavigationView*, const NavigationViewItemInvokedEventArgs& args) mutable {
            if (args.IsSettingsInvoked) {
                if (win) StopStreamingThread();
                nav->SetContent(settingsContent);
                return;
            }
            if (!args.InvokedItem) return;

            const std::string tag = args.InvokedItem->GetTag();
            if (tag.empty()) return;

            auto it = contentByTag.find(tag);
            if (it == contentByTag.end()) return;

            nav->SetContent(it->second);
            if (win) {
                if (tag == "stream") StartStreamingThread(win, streamImg);
                else StopStreamingThread();
            }
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

        // Root chrome/content must not use the default Column gap. ToastCenter is
        // a zero-size overlay host; if it participates in a gapped Column, the
        // flex layout still reserves an 8px gap before it and the main content
        // stops short of the bottom edge on the first layout.
        return Column(0).Children({
            titleBar,
            Expanded(GalleryNavigation(effectiveContext, context).Build()),
            toastCenter
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
