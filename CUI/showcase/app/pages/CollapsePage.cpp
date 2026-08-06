#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildCollapsePage(const ShowcaseContext& ctx) {
    auto panel1 = CollapsePanelWidget("1. 基础视图配置 (Basic Settings)")
        .Subtitle("包含常规同步策略与状态栏消息提醒管理")
        .Build();
    panel1->SetContent(Column(8).Children({
        CheckboxTile("启用自动同步云端配置文件").Build(),
        CheckboxTile("显示系统底栏状态通知").Build(),
        ElevatedButton("应用当前基础配置").Width(160).Height(28).Build()
    }).Build());

    auto panel2 = CollapsePanelWidget("2. 高级硬件加速选项 (Advanced Options)")
        .Subtitle("调整 Direct2D 独立多线程硬件渲染与缓冲池配置")
        .Build();
    panel2->SetContent(Column(8).Children({
        ToggleSwitchTile("开启 Direct2D 独立多线程硬件渲染", true).Build(),
        SliderWidget(75.0f).Width(280).Height(24).Build()
    }).Build());

    auto panel3 = CollapsePanelWidget("3. 开发者调试与日志 (Developer Logs)")
        .Subtitle("开启实时渲染帧率 overlay 与事件流调试日志")
        .Build();
    panel3->SetContent(Column(8).Children({
        CheckboxTile("开启 FPS 帧率渲染叠加层 (Overlay)").Build(),
        CheckboxTile("记录详细 DirectWrite 文本度量与排版 Debug 日志").Build(),
        ElevatedButton("导出调试报告 (Export Diagnostics)").Width(200).Height(28).Build()
    }).Build());

    return { "CollapsePanel 折叠面板", CreatePage(
        "CollapsePanel 可折叠手风琴面板控件",
        "全新的 Fluent UI 风格手风琴面板：右侧 2D 矢量 Chevron 箭头随展开动画平滑旋转 180°，结合卡片圆角、灰度渐变分割线与副标题支持。",
        CreateDemoSurface({ panel1, panel2, panel3 }),
        CreatePropertyGrid(ctx, panel1), panel1) };
}
