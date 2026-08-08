#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildCollapsePage(const ShowcaseContext& ctx) {
    auto panel1 = ExpanderWidget("1. 基础视图配置 (Basic Settings)")
        .Subtitle("包含常规同步策略与状态栏消息提醒管理")
        .Build();
    panel1->SetContent(Column(8).Children({
        CheckboxTile("启用自动同步云端配置文件").Build(),
        CheckboxTile("显示系统底栏状态通知").Build(),
        ElevatedButton("应用当前基础配置").Width(160).Height(28).Build()
    }).Build());

    auto panel2 = ExpanderWidget("2. 高级硬件加速选项 (Advanced Options)")
        .Subtitle("调整 Direct2D 独立多线程硬件渲染与缓冲池配置")
        .Build();
    panel2->SetContent(Column(8).Children({
        ToggleSwitchTile("开启 Direct2D 独立多线程硬件渲染", true).Build(),
        SliderWidget(75.0f).Width(280).Height(24).Build()
    }).Build());

    auto panel3 = ExpanderWidget("3. 开发者调试与日志 (Developer Logs)")
        .Subtitle("开启实时渲染帧率 overlay 与事件流调试日志")
        .Build();
    panel3->SetContent(Column(8).Children({
        CheckboxTile("开启 FPS 帧率渲染叠加层 (Overlay)").Build(),
        CheckboxTile("记录详细 DirectWrite 文本度量与排版 Debug 日志").Build(),
        ElevatedButton("导出调试报告 (Export Diagnostics)").Width(200).Height(28).Build()
    }).Build());

    return { "Expander 折叠面板", CreatePage(
        "Expander (WinUI 3)",
        "参照 WinUI 3 Expander：固定 Header + 可折叠 Content，展开时推开布局而非浮层；支持 IsExpanded、ExpandDirection、Expanding/Collapsed 事件。",
        CreateDemoSurface({ panel1, panel2, panel3 }),
        CreatePropertyGrid(ctx, panel1), panel1) };
}
