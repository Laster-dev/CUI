#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/TabView.h"
#include <format>
#include <memory>
#include <string>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<UIElement> MakeTabPage(const std::string& title, const std::string& body) {
    return Column(12, {
        Text(title).FontSize(16.0f).FontWeight(FontWeight::SemiBold),
        Text(body),
        Row(8, {
            ElevatedButton("示例操作", [](UIElement*) {}).Build(),
            ElevatedButton("刷新", [](UIElement*) {}).Build(),
        }),
        MakeStatus("每个标签页持有独立的内容树，切换时互不干扰。"),
    }).Build();
}

} // anonymous namespace

Element BuildTabViewPage() {
    // ---------- 1. 常规用法 ----------
    auto tabView = std::make_shared<TabView>();
    tabView->SetWidth(560.0f);
    tabView->SetHeight(300.0f);
    tabView->AddTab("首页", MakeTabPage("首页", "TabView 支持多标签页切换、关闭与横向滚动。"), "🏠", true);
    tabView->AddTab("文档", MakeTabPage("文档", "每个标签可绑定独立内容树，切换时保留各自状态。"), "📄", true);
    tabView->AddTab("设置", MakeTabPage("设置", "点击标签右侧的 × 即可关闭标签。"), "⚙️", true);
    tabView->AddTab("关于", MakeTabPage("关于", "标签过多时头部可横向滚动。"), "ℹ️", true);
    tabView->SetSelectedIndex(0);

    auto status1 = MakeStatus("当前选中: [首页] (索引 0)");
    tabView->OnSelectionChanged().Connect([status1](TabView*, int index) {
        status1->Text = std::format("选中已切换 → 索引 {}", index);
    });
    tabView->OnTabClosed().Connect([status1](TabView*, int index) {
        status1->Text = std::format("标签已关闭（索引 {}）", index);
    });

    // ---------- 2. 动态管理 ----------
    auto dynTabs = std::make_shared<TabView>();
    dynTabs->SetWidth(560.0f);
    dynTabs->SetHeight(280.0f);
    dynTabs->AddTab("初始页", MakeTabPage("初始页", "通过右侧按钮动态添加 / 关闭 / 跳转标签。"), "📌", true);
    dynTabs->SetSelectedIndex(0);

    auto status2 = MakeStatus("动态添加、关闭与程序化切换标签。");
    dynTabs->OnTabClosed().Connect([status2](TabView*, int) {
        status2->Text = "已关闭一个标签。";
    });

    auto tabCounter = std::make_shared<int>(1);
    auto btnAdd = ElevatedButton("添加标签", [dynTabs, tabCounter, status2](UIElement*) {
        const int n = (*tabCounter)++;
        dynTabs->AddTab(std::format("新标签 {}", n),
                        MakeTabPage(std::format("新标签 {}", n), "这是动态添加的标签页内容。"),
                        "✨", true);
        dynTabs->SetSelectedIndex(dynTabs->GetSelectedIndex() + 1);
        status2->Text = std::format("已添加并选中 [新标签 {}]", n);
    }).Build();
    auto btnCloseSel = ElevatedButton("关闭选中", [dynTabs, status2](UIElement*) {
        const int idx = dynTabs->GetSelectedIndex();
        if (idx >= 0) {
            dynTabs->RemoveTab(idx);
            status2->Text = std::format("已关闭选中标签（索引 {}）", idx);
        }
    }).Build();
    auto btnGoFirst = ElevatedButton("跳转到第 1 个", [dynTabs, status2](UIElement*) {
        if (dynTabs->GetSelectedIndex() != 0) {
            dynTabs->SetSelectedIndex(0);
            status2->Text = "已程序化切换到第 1 个标签。";
        }
    }).Build();
    auto btnWider = ElevatedButton("加宽标签 (Max 300)", [dynTabs](UIElement*) {
        dynTabs->SetMaxTabWidth(300.0f);
    }).Build();
    auto btnNarrow = ElevatedButton("收窄标签 (Min 60)", [dynTabs](UIElement*) {
        dynTabs->SetMinTabWidth(60.0f);
    }).Build();

    // ---------- 3. 固定标签（不可关闭） ----------
    auto pinned = std::make_shared<TabView>();
    pinned->SetWidth(560.0f);
    pinned->SetHeight(260.0f);
    pinned->AddTab("固定页 (不可关闭)", MakeTabPage("固定页", "isClosable = false：标签右侧不显示 ×，无法关闭。"), "🔒", false);
    pinned->AddTab("普通页", MakeTabPage("普通页", "isClosable = true：可正常关闭。"), "📄", true);
    pinned->AddTab("工作区", MakeTabPage("工作区", "混合固定 / 可关闭标签，适合常驻面板。"), "🖥️", true);
    pinned->SetSelectedIndex(0);

    auto status3 = MakeStatus("固定标签不渲染关闭按钮，也无法被 RemoveTab 关闭。");
    pinned->OnTabClosed().Connect([status3](TabView*, int index) {
        status3->Text = std::format("可关闭标签被关闭（索引 {}）", index);
    });

    SamplePageSpec spec;
    spec.title = "TabView (标签页视图)";
    spec.subtitle = "WinUI 风格多标签页：切换、关闭、溢出横向滚动、选中指示动画，支持动态增删与不可关闭的固定标签。";
    spec.sections = {
        {
            "常规用法 — 多标签切换",
            "1. AddTab 绑定标题、图标与独立内容树；\n"
            "2. 点击标签切换选中，指示条平滑滑动；点击 × 关闭标签；\n"
            "3. OnSelectionChanged / OnTabClosed 事件回调携带索引。",
            Column(12, {
                tabView,
                status1,
            }),
        },
        {
            "动态管理 — 添加 / 关闭 / 跳转",
            "1. 运行时 AddTab 追加新标签并自动选中；\n"
            "2. RemoveTab 关闭当前选中；SetSelectedIndex 程序化跳转；\n"
            "3. SetMinTabWidth / SetMaxTabWidth 调整标签宽度。",
            Column(12, {
                Row(8, { btnAdd, btnCloseSel, btnGoFirst, btnWider, btnNarrow }),
                dynTabs,
                status2,
            }),
        },
        {
            "固定标签（不可关闭）",
            "isClosable = false 的标签不渲染 × 按钮；固定标签常驻，适合首页 / 工作区等面板。",
            Column(12, {
                pinned,
                status3,
            }),
        },
    };

    spec.source = R"(
// 1) 创建标签视图并添加标签
auto tabs = std::make_shared<TabView>();
tabs->AddTab("首页", pageContent, "🏠", true);
tabs->AddTab("固定页", pinnedContent, "🔒", false); // 不可关闭

// 2) 事件
tabs->OnSelectionChanged().Connect([](TabView*, int index) { });
tabs->OnTabClosed().Connect([](TabView*, int index) { });

// 3) 动态操作
tabs->AddTab("新标签", content);
tabs->RemoveTab(0);
tabs->SetSelectedIndex(1);
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery
