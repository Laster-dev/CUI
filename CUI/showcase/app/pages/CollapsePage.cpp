#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildCollapsePage(const ShowcaseContext& ctx) {
    auto panel1 = CollapsePanelWidget("1. 基础视图配置 (Basic Settings)").Build();
    panel1->SetContent(Column(8).Children({
        CheckboxTile("启用自动同步云端配置文件").Build(),
        CheckboxTile("显示系统底栏状态通知").Build(),
        ElevatedButton("应用当前基础配置").Width(160).Height(26).Build()
    }).Build());
    auto panel2 = CollapsePanelWidget("2. 高级硬件加速选项 (Advanced Options)").Build();
    panel2->SetContent(Column(8).Children({
        ToggleSwitchTile("开启 Direct2D 独立多线程硬件渲染", true).Build(),
        SliderWidget(75.0f).Width(240).Height(24).Build()
    }).Build());

    return { "CollapsePanel 折叠面板", CreatePage(
        "CollapsePanel 可折叠手风琴面板控件",
        "支持点击标题栏展开/收起子容器、自定义 Header 文本与动画过渡。",
        CreateDemoSurface({ panel1, panel2 }),
        CreatePropertyGrid(ctx, panel1), panel1) };
}
