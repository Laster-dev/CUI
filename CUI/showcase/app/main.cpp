#include "ShowcaseContext.h"
#include "Streaming.h"
#include "pages/PageRegistry.h"
#include "framework/window/Window.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/ToastCenter.h"
#include "framework/controls/VSCodeControls.h"
#include <shellscalingapi.h>
#include <iostream>
#include <vector>

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

class GalleryTabs : public StatelessWidget {
public:
    explicit GalleryTabs(const ShowcaseContext& ctx, BuildContext buildContext = {})
        : StatelessWidget(buildContext), m_ctx(ctx) {}

    std::shared_ptr<UIElement> build(BuildContext& context) override {
        auto tabView = std::make_shared<TabView>();
        auto pages = std::vector<ShowcasePage>{
            BuildButtonPage(m_ctx),
            BuildTextBlockPage(m_ctx),
            BuildTextBoxPage(m_ctx),
            BuildPasswordBoxPage(m_ctx),
            BuildCheckBoxPage(m_ctx),
            BuildRadioButtonPage(m_ctx),
            BuildToggleSwitchPage(m_ctx),
            BuildComboBoxPage(m_ctx),
            BuildSliderPage(m_ctx),
            BuildProgressBarPage(m_ctx),
            BuildNumberBoxPage(m_ctx),
            BuildDatePickerPage(m_ctx),
            BuildTimePickerPage(m_ctx),
            BuildColorPickerPage(m_ctx),
            BuildBreadcrumbPage(m_ctx),
            BuildPagingPage(m_ctx),
            BuildSplitterPage(m_ctx),
            BuildCollapsePage(m_ctx),
            BuildListBoxPage(m_ctx),
            BuildListViewPage(m_ctx),
            BuildTreeViewPage(m_ctx),
            BuildHyperlinkPage(m_ctx),
            BuildStreamPage(m_ctx),
            BuildDialogPage(m_ctx),
            BuildToastPage(m_ctx),
            BuildGridPage(m_ctx),
            BuildCanvasPage(m_ctx),
            BuildWrapPage(m_ctx),
            BuildDockPage(m_ctx),
            BuildUniformPage(m_ctx),
            BuildStackPanelPage(m_ctx),
            BuildScrollViewerPage(m_ctx),
            BuildFlyoutPage(m_ctx),
            BuildNavigationViewPage(m_ctx)
        };

        for (const auto& page : pages) {
            tabView->AddTab(page.tabTitle, page.content);
        }

        auto effectiveWindow = context.window ? context.window : m_ctx.windowRef;
        tabView->OnSelectionChanged().Connect([ctx = m_ctx, effectiveWindow](TabView*, int idx) mutable {
            ctx.windowRef = effectiveWindow;
            if (idx == 21) StartStreamingThread(ctx.windowRef, ctx.streamImage);
            else StopStreamingThread();
        });

        return tabView;
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
        titleBar->SetProperty("title", Value("CUI Control Gallery | Flutter 风格 C++ 声明式 Widget Showcase"));

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
            Expanded(GalleryTabs(effectiveContext, context).Build()),
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
