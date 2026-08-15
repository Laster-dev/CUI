#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/core/Value.h"
#include "framework/controls/ComboBox.h"
#include "framework/controls/ToggleSwitch.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<CUI::Button> MakeDockBar(
    const std::string& text,
    D2D1_COLOR_F color,
    Dock dock,
    float mainSize = 0.0f) {
    auto bar = ElevatedButton(text).Background(color).Build();
    bar->SetDock(dock);
    if (dock == Dock::Left || dock == Dock::Right) {
        bar->Width = mainSize > 0.0f ? mainSize : 90.0f;
    } else {
        bar->Height = mainSize > 0.0f ? mainSize : 32.0f;
    }
    return bar;
}

} // namespace

std::shared_ptr<UIElement> BuildDockPanelPage() {
    // —— 常规用法：四边停靠 + 中央填充 ——
    auto classic = DockPanelWidget().Width(520).Height(240).Build();
    classic->AddChild(MakeDockBar("Top 顶栏", Rgb(0x007ACC), Dock::Top, 36));
    classic->AddChild(MakeDockBar("Left 侧栏", Rgb(0x845EF7), Dock::Left, 110));
    classic->AddChild(MakeDockBar("Right 侧栏", Rgb(0xF59F00), Dock::Right, 90));
    classic->AddChild(MakeDockBar("Bottom 底栏", Rgb(0x10B981), Dock::Bottom, 32));
    classic->AddChild(MakeDockBar("Center 填充", Rgb(0xD13438), Dock::Left));

    // —— 停靠顺序对结果的影响 ——
    auto orderA = DockPanelWidget().Width(252).Height(200).Build();
    orderA->AddChild(MakeDockBar("Left 先", Rgb(0x845EF7), Dock::Left, 110));
    orderA->AddChild(MakeDockBar("Top", Rgb(0x007ACC), Dock::Top, 32));
    orderA->AddChild(MakeDockBar("剩余填充", Rgb(0xD13438), Dock::Left));

    auto orderB = DockPanelWidget().Width(252).Height(200).Build();
    orderB->AddChild(MakeDockBar("Top 先", Rgb(0x007ACC), Dock::Top, 32));
    orderB->AddChild(MakeDockBar("Left", Rgb(0x845EF7), Dock::Left, 110));
    orderB->AddChild(MakeDockBar("剩余填充", Rgb(0xD13438), Dock::Left));

    // —— LastChildFill 实时开关 ——
    auto fillDock = DockPanelWidget().Width(420).Height(180).Build();
    fillDock->AddChild(MakeDockBar("Left 侧栏", Rgb(0x845EF7), Dock::Left, 100));
    fillDock->AddChild(MakeDockBar("Right 侧栏", Rgb(0xF59F00), Dock::Right, 80));
    fillDock->AddChild(MakeDockBar("最后一项 · Bottom 36px", Rgb(0xD13438), Dock::Bottom, 36));

    auto fillToggle = ToggleSwitchWidget().Build();
    State<bool> fillState{ true };
    fillToggle->IsOn.Bind(fillState);
    fillDock->LastChildFill.Bind(fillState, BindingMode::OneWay);

    auto fillStatus = MakeStatus("");
    fillStatus->Text.Bind(MakeComputed<std::string>([](bool fill) {
        return fill
            ? "LastChildFill = true：最后一项忽略自身 Dock 方位，拉伸填满剩余区域。"
            : "LastChildFill = false：最后一项按自身 Dock 停靠（本例为 Bottom，只占底部 36px）。";
    }, fillState), BindingMode::OneWay);

    // —— 运行时动态切换停靠方位 ——
    auto liveDock = DockPanelWidget().Width(420).Height(200).Build();
    auto hero = MakeDockBar("主角元素", Rgb(0x007ACC), Dock::Left, 100);
    liveDock->AddChild(hero);
    liveDock->AddChild(MakeDockBar("剩余填充区", Rgb(0x2F3A46), Dock::Left));

    auto dockCombo = ComboBoxWidget();
    dockCombo->AddItem("Left（贴左，宽 100px）");
    dockCombo->AddItem("Top（贴顶，高 44px）");
    dockCombo->AddItem("Right（贴右，宽 100px）");
    dockCombo->AddItem("Bottom（贴底，高 44px）");
    dockCombo->SetSelectedIndex(0);

    State<int> dockIndex{ 0 };
    dockCombo->SelectedIndex.Bind(dockIndex);

    // 切换停靠方位是批量布局命令（改 Dock + 尺寸 + 颜色），保留事件驱动。
    auto applyDock = [hero](int index) {
        const Dock docks[] = { Dock::Left, Dock::Top, Dock::Right, Dock::Bottom };
        const D2D1_COLOR_F colors[] = { Rgb(0x007ACC), Rgb(0x10B981), Rgb(0xF59F00), Rgb(0x845EF7) };
        hero->SetDock(docks[index]);
        hero->Background = colors[index];
        if (docks[index] == Dock::Left || docks[index] == Dock::Right) {
            hero->Width = 100.0f;
            hero->Height = -1.0f;
        } else {
            hero->Height = 44.0f;
            hero->Width = -1.0f;
        }
    };
    dockCombo->OnSelectionChanged().Connect([applyDock](ComboBox*, int index, const std::string&) {
        applyDock(index);
    });

    auto dockStatus = MakeStatus("");
    dockStatus->Text.Bind(MakeComputed<std::string>([](int index) {
        switch (index) {
            case 1: return std::string("主角当前停靠：Top —— 占据顶部整行 44px。");
            case 2: return std::string("主角当前停靠：Right —— 贴右边缘，宽 100px。");
            case 3: return std::string("主角当前停靠：Bottom —— 贴底边，高 44px。");
            default: return std::string("主角当前停靠：Left —— 贴左边缘，宽 100px。");
        }
    }, dockIndex), BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "DockPanel(停靠面板)";
    spec.subtitle = "将子元素依次停靠在容器的四条边上，最后一个子元素默认拉伸填满剩余区域。";
    spec.sections = {
        {
            "常规用法（四边停靠）",
            "按添加顺序依次切割边缘：Top 先占顶部整行，Left/Right 各占左右整列，Bottom 再占底部，最后一项填充中心。",
            Column(12, {
                classic,
                MakeStatus("顺序为 Top → Left → Right → Bottom → 填充；先停靠的边占据整条边，后停靠的边在剩余区域内缩进。"),
            }),
        },
        {
            "停靠顺序影响布局",
            "同一个 DockPanel，子元素的添加顺序不同，结果完全不同：先停靠 Left 时它占满整列高度，先停靠 Top 时它占满整行宽度。",
            Row(12, {
                Column(8, { orderA, MakeStatus("先停靠 Left：左栏占满整列，Top 只在剩余区域内。") }),
                Column(8, { orderB, MakeStatus("先停靠 Top：顶栏占满整行，Left 在顶栏之下。") }),
            }),
        },
        {
            "LastChildFill 实时开关",
            "SetLastChildFill(false) 后，最后一项不再填满剩余区域，而是按自己的 Dock 方位正常停靠。",
            Column(12, {
                fillDock,
                WrapPanelWidget("Horizontal").Gap(16).Children({ fillToggle, fillStatus }).Build(),
            }),
        },
        {
            "运行时动态切换停靠方位",
            "通过 SetDock 在运行时把同一个元素停靠到任意一边，布局立即重新切割；状态文字由 SelectedIndex 状态派生。",
            Column(12, {
                liveDock,
                WrapPanelWidget("Horizontal").Gap(16).Children({ dockCombo, dockStatus }).Build(),
            }),
        },
    };
    spec.source =
        "auto dock = DockPanelWidget().Width(520).Height(240).Build();\n"
        "auto top = ElevatedButton(\"顶栏\").Build();\n"
        "top->SetDock(Dock::Top);      // 停靠到顶部\n"
        "dock->AddChild(top);\n"
        "\n"
        "auto left = ElevatedButton(\"侧栏\").Build();\n"
        "left->SetDock(Dock::Left);\n"
        "dock->AddChild(left);\n"
        "\n"
        "// 最后添加的子元素默认填满剩余区域\n"
        "dock->AddChild(ElevatedButton(\"中央填充\").Build());\n"
        "\n"
        "dock->SetLastChildFill(false); // 关闭后最后一项按自身 Dock 停靠\n"
        "item->SetDock(Dock::Right);     // 运行时切换停靠方位，布局自动重排\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
