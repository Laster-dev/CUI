#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include "framework/devtools/DevToolsManager.h"
#include "framework/devtools/PixelSnapshotTest.h"
#include <format>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildDevToolsPage() {
    auto statusLabel = MakeStatus("快捷键就绪：按 F12 呼出检查器，Ctrl+Shift+C 审查元素，Ctrl+Shift+P 切换 HUD，Ctrl+Shift+D 切换重绘热力图");

    // 1. 检查器控制按钮
    auto btnToggleDevTools = Widgets::Button()
        .Text("🛠 呼出 F12 检查面板")
        .OnClick([statusLabel](UIElement*) {
            DevToolsManager::Instance().ToggleDevTools();
            statusLabel->Text = DevToolsManager::Instance().IsInspectorVisible()
                ? "DevTools 检查面板已【开启】(可拖动标题栏移动位置)"
                : "DevTools 检查面板已【关闭】";
        })
        .Shared();

    auto btnToggleInspect = Widgets::Button()
        .Text("🔍 元素拾取审查 (Ctrl+Shift+C)")
        .OnClick([statusLabel](UIElement*) {
            DevToolsManager::Instance().ToggleInspectMode();
            statusLabel->Text = DevToolsManager::Instance().IsInspectMode()
                ? "【审查模式激活】在界面上移动鼠标即可高亮盒模型，点击任意元素即可在 DevTools 中选中！"
                : "【审查模式关闭】";
        })
        .Shared();

    auto btnToggleBoxModel = Widgets::Button()
        .Text("📦 盒模型高亮开关")
        .OnClick([statusLabel](UIElement*) {
            DevToolsManager::Instance().ToggleBoxModelOverlay();
            statusLabel->Text = DevToolsManager::Instance().IsBoxModelOverlayEnabled()
                ? "盒模型诊断遮罩已【启用】"
                : "盒模型诊断遮罩已【停用】";
        })
        .Shared();

    // 2. 盒模型测试靶标控件 (带明确的 Margin、Padding 与 Border，供用户直接拾取体验)
    auto targetCard = Column(8.0f, {
        Widgets::TextBlock()
            .Text("📦 盒模型审查靶标 (Box Model Test Target)")
            .FontWeight(FontWeight::Bold)
            .Shared(),
        Widgets::TextBlock()
            .Text("本卡片预设了明确的 Margin (12, 8, 12, 8)、Border (2.0) 与 Padding (16, 12, 16, 12)。按下上方【🔍 元素拾取审查】后将光标移至此处，即可直观观察四层色彩分明的 Chrome DevTools 盒模型。")
            .Shared(),
        Row(8.0f, {
            Widgets::Button().Text("嵌套子按钮 A").Shared(),
            Widgets::Button().Text("嵌套子按钮 B").Shared(),
        }),
    });
    targetCard->ApplyMargin(Thickness(12.0f, 8.0f, 12.0f, 8.0f));
    targetCard->ApplyPadding(Thickness(16.0f, 12.0f, 16.0f, 12.0f));
    targetCard->ApplyBorderThickness(2.0f);

    // 3. 性能与重绘控制按钮
    auto btnToggleHud = Widgets::Button()
        .Text("⚡ 切换性能 HUD (Ctrl+Shift+P)")
        .OnClick([statusLabel](UIElement*) {
            DevToolsManager::Instance().TogglePerformanceOverlay();
            statusLabel->Text = DevToolsManager::Instance().IsPerformanceOverlayEnabled()
                ? "实时性能 Profiler HUD 已【开启】(右上角查看 FPS 与 60 帧 Sparkline 走势图)"
                : "性能 Profiler HUD 已【关闭】";
        })
        .Shared();

    auto btnToggleOverdraw = Widgets::Button()
        .Text("📊 切换重绘热力图 (Ctrl+Shift+D)")
        .OnClick([statusLabel](UIElement*) {
            DevToolsManager::Instance().ToggleOverdrawVisualizer();
            statusLabel->Text = DevToolsManager::Instance().IsOverdrawVisualizerEnabled()
                ? "重绘脏区热力图已【开启】(绿/蓝/黄/红色彩实时高亮重绘区域)"
                : "重绘脏区热力图已【关闭】";
        })
        .Shared();

    // 动态重绘测试控件 (用于测试 Overdraw 热力图闪烁)
    auto triggerCounter = std::make_shared<int>(0);
    auto testBadge = MakeStatus("重绘测试基准点: 点击右侧按钮产生局部脏区刷新");
    auto btnTriggerRedraw = Widgets::Button()
        .Text("🔄 触发局部重绘")
        .OnClick([triggerCounter, testBadge](UIElement*) {
            (*triggerCounter)++;
            testBadge->Text = std::format("已触发局部重绘 #{} 次 (若已开热力图可观察到高亮)", *triggerCounter);
        })
        .Shared();

    // 4. 自动化像素快照比对测试
    auto snapshotReportLabel = MakeStatus("快照回归测试状态：点击下方按钮执行离屏栅格化与像素 Diff 校验");
    auto btnRunSnapshotTest = Widgets::Button()
        .Text("📷 执行控件像素快照比对测试")
        .OnClick([snapshotReportLabel, targetCard](UIElement*) {
            if (!Window::Current()) return;
            auto& ctx = Window::Current()->GetGraphicsContext();

            // 生成基准快照与测试候选快照
            SnapshotBuffer golden = PixelSnapshotTest::CaptureElement(targetCard.get(), Size(400.0f, 120.0f), ctx);
            SnapshotBuffer candidate = PixelSnapshotTest::CaptureElement(targetCard.get(), Size(400.0f, 120.0f), ctx);

            SnapshotDiffReport report = PixelSnapshotTest::Compare(golden, candidate);

            snapshotReportLabel->Text = std::format(
                "像素校验结果: {} | 像素总数: {} | 差异像素: {} (差异率: {:.2f}%) | 单像素最大色差: {:.3f}",
                report.passed ? "✔ PASSED (100% 匹配)" : "❌ FAILED",
                report.totalPixels,
                report.diffPixels,
                report.diffRatio * 100.0f,
                report.maxDelta
            );
        })
        .Shared();

    SamplePageSpec spec;
    spec.title = "DevTools & Runtime Inspector (开发者工具与运行时诊断)";
    spec.subtitle = "框架原生内置的运行时可视化检查套件，提供类似 Chrome DevTools 的全功能元素树查看器、盒模型 4 层同心遮罩、性能 FPS/耗时折线图 HUD、重绘脏区热力图以及自动化像素快照回归测试。";
    spec.sections = {
        {
            "可视树检查器与盒模型审查 (Visual Tree & Box Model)",
            "按下 F12 或快捷键直接唤出调试浮动面板；开启审查模式后鼠标划过任意节点即呈现 Margin (外边距, 橙)、Border (边框, 黄)、Padding (内边距, 绿)、Content (内容, 蓝) 与规格 Badge。",
            Column(12, {
                Row(8, { btnToggleDevTools, btnToggleInspect, btnToggleBoxModel }),
                targetCard,
            }),
        },
        {
            "性能监控与重绘热力图 (Performance HUD & Overdraw Visualizer)",
            "量化诊断卡顿与冗余重绘：实时 FPS 追踪、布局与绘制耗时分解、DisplayList 指令统计、最近 60 帧平滑 Sparkline 走势图，以及多级颜色警示重绘热力图。",
            Column(12, {
                Row(8, { btnToggleHud, btnToggleOverdraw }),
                Row(8, { btnTriggerRedraw, testBadge }),
            }),
        },
        {
            "自动化像素快照与基准回归 (Golden Image Testing)",
            "借助 PixelSnapshotTest 在离屏管线栅格化目标 UI 元素并进行逐像素 Diff 校验，自动过滤亚像素抖动并生成差异热力图，杜绝重构退化。",
            Column(12, {
                Row(8, { btnRunSnapshotTest }),
                snapshotReportLabel,
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 快捷键与便捷开关
// F12 或 Ctrl+Shift+I : 切换 DevTools 检查面板
// Ctrl+Shift+C        : 切换元素拾取审查模式
// Ctrl+Shift+P        : 切换性能与帧率仪表盘 HUD
// Ctrl+Shift+D        : 切换重绘脏区热力图

// 2. 纯代码直接调用 DevTools API
DevToolsManager::Instance().ToggleDevTools();
DevToolsManager::Instance().ToggleInspectMode();
DevToolsManager::Instance().TogglePerformanceOverlay();
DevToolsManager::Instance().ToggleOverdrawVisualizer();

// 3. 自动化离屏像素快照与 Golden Image 比对
SnapshotBuffer golden = PixelSnapshotTest::CaptureElement(element, Size(400, 200), ctx);
SnapshotBuffer candidate = PixelSnapshotTest::CaptureElement(element, Size(400, 200), ctx);
SnapshotDiffReport report = PixelSnapshotTest::Compare(golden, candidate, 0.03f);
if (report.passed) {
    // 视觉像素校验 100% 通过
}
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
