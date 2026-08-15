#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/CommandBar.h"
#include "framework/controls/TextBox.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 CommandBar (命令栏) 展示页面。
 * 遵循 Fluent 现代规范，全弹性流式布局，不硬编码固定宽高。
 */
Element BuildCommandBarPage() {
    // --- 状态与交互反馈面板 ---
    auto statusLabel = MakeStatus("点击命令栏中的按钮、开关或溢出菜单项查看实时联动。");

    // ==========================================
    // 示例 1: 经典富文本文档编辑命令栏
    // ==========================================
    auto docCmdBar = CommandBarWidget().Build();

    // 1. 主要操作按钮 (Primary Action Buttons)
    auto btnNew = docCmdBar->AddButton("新建", "📄");
    btnNew->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "已触发命令：【新建文档】 (已初始化空模板)";
    });

    auto btnOpen = docCmdBar->AddButton("打开", "📂");
    btnOpen->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "已触发命令：【打开文件】 (正在浏览工作区)";
    });

    auto btnSave = docCmdBar->AddButton("保存", "💾");
    btnSave->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "已触发命令：【保存当前文件】 (文件已同步至磁盘)";
    });

    docCmdBar->AddSeparator();

    // 2. 撤销 / 重做
    auto btnUndo = docCmdBar->AddButton("撤销", "↩️");
    btnUndo->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "已触发命令：【撤销 (Undo)】";
    });

    auto btnRedo = docCmdBar->AddButton("重做", "↪️");
    btnRedo->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "已触发命令：【重做 (Redo)】";
    });

    docCmdBar->AddSeparator();

    // 3. 样式开关按钮 (ToggleButtons)
    auto toggleBold = docCmdBar->AddToggle("加粗", "𝐁");
    toggleBold->OnClick.Connect([statusLabel, toggleBold](UIElement*) {
        statusLabel->Text = std::format("字体加粗样式已切换为: {}", toggleBold->IsChecked() ? "开启 [ON]" : "关闭 [OFF]");
    });

    auto toggleItalic = docCmdBar->AddToggle("斜体", "𝐼");
    toggleItalic->OnClick.Connect([statusLabel, toggleItalic](UIElement*) {
        statusLabel->Text = std::format("字体斜体样式已切换为: {}", toggleItalic->IsChecked() ? "开启 [ON]" : "关闭 [OFF]");
    });

    auto toggleUnderline = docCmdBar->AddToggle("下划线", "𝐔");
    toggleUnderline->OnClick.Connect([statusLabel, toggleUnderline](UIElement*) {
        statusLabel->Text = std::format("字体下划线样式已切换为: {}", toggleUnderline->IsChecked() ? "开启 [ON]" : "关闭 [OFF]");
    });

    // 4. 次要菜单项 (Secondary Overflow Items)
    auto secProps = docCmdBar->AddSecondary("文档属性", "ℹ️");
    secProps->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "已打开【文档元数据与属性】检查器。";
    });

    auto secPrint = docCmdBar->AddSecondary("打印预览", "🖨️");
    secPrint->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "已启动【打印机驱动与预览】对话框。";
    });

    auto secPdf = docCmdBar->AddSecondary("导出为 PDF", "📑");
    secPdf->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "正在生成并导出为【PDF 格式文件】...";
    });

    docCmdBar->AddSecondarySeparator();

    auto secPageLayout = docCmdBar->AddSecondary("页面布局设置", "⚙️");
    secPageLayout->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "已打开【页面边距与纸张方向】设置。";
    });

    // 模拟文档编辑展示卡片
    auto mockEditor = TextField()
        .Text("CommandBar（命令栏）是 Fluent 风格中用于组织页面或局部上下文核心操作的工具条。\n"
              "它能够智能感知可用宽度，当空间不足时自动将次要操作以及溢出的主要操作收纳进右侧的【更多 (…)】下拉菜单中。\n"
              "支持普通动作按钮、状态开关按钮、水平分隔线以及次要功能项。")
        .Height(100.0f);
    mockEditor->SetAcceptsReturn(true);
    mockEditor->SetTextWrapping(true);

    // ==========================================
    // 示例 2: 标签显示模式切换 (Label Position)
    // ==========================================
    auto mediaCmdBar = CommandBarWidget().Build();

    auto btnPlay = mediaCmdBar->AddButton("播放", "▶️");
    btnPlay->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "媒体控制器：【开始播放】";
    });
    auto btnPause = mediaCmdBar->AddButton("暂停", "⏸️");
    btnPause->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "媒体控制器：【暂停播放】";
    });
    auto btnForward = mediaCmdBar->AddButton("快进", "⏩");
    btnForward->OnClick.Connect([statusLabel](UIElement*) {
        statusLabel->Text = "媒体控制器：【快进 10 秒】";
    });
    mediaCmdBar->AddSeparator();
    auto loopToggle = mediaCmdBar->AddToggle("循环", "🔁");
    loopToggle->OnClick.Connect([statusLabel, loopToggle](UIElement*) {
        statusLabel->Text = std::format("循环播放模式: {}", loopToggle->IsChecked() ? "单曲循环" : "列表播放");
    });
    mediaCmdBar->AddSecondary("均衡器配置", "🎚️");
    mediaCmdBar->AddSecondary("音轨选择", "🎵");

    auto btnToggleMode = ToggleButtonWidget("切换为仅图标模式 (Collapsed)");
    btnToggleMode->OnClick.Connect([mediaCmdBar, statusLabel](UIElement* sender) {
        auto btn = dynamic_cast<ToggleButton*>(sender);
        bool iconOnly = btn && btn->IsChecked();
        mediaCmdBar->SetLabelPosition(iconOnly ? CommandBarLabelPosition::Collapsed : CommandBarLabelPosition::Right);
        statusLabel->Text = iconOnly ? "当前命令栏显示模式：仅图标紧凑模式 (Collapsed)" : "当前命令栏显示模式：文字与图标并排模式 (Right)";
    });

    SamplePageSpec spec;
    spec.title = "CommandBar (命令栏)";
    spec.subtitle = "提供流式弹性排布、智能宽度响应、自动折叠收纳溢出项与多类型动作按钮组合的现代工具栏。";
    spec.sections = {
        {
            "富文本编辑命令栏 (Primary / Toggle / Secondary 溢出收纳)",
            "展示动作按钮、加粗/斜体开关按钮、分隔线以及点击右侧【…】弹出的次要操作菜单。在窄屏下超出宽度的按钮将自动迁移至溢出菜单中。",
            Column(12, {
                docCmdBar,
                mockEditor,
            }),
        },
        {
            "显示模式切换与紧凑排版",
            "支持在图标+文字（Right）与纯图标模式（Collapsed）之间无缝切换，适应不同密度的桌面工作流。",
            Column(12, {
                Row(8, { btnToggleMode }),
                mediaCmdBar,
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建命令栏并添加操作
auto cmdBar = CommandBarWidget().Build();

auto btnNew = cmdBar->AddButton("新建", "📄");
btnNew->OnClick.Connect([](UIElement*) { /* ... */ });

// 2. 添加开关型按钮
auto toggleBold = cmdBar->AddToggle("加粗", "𝐁");

// 3. 添加次要菜单项（收纳于 ... 溢出菜单）
cmdBar->AddSecondary("导出为 PDF", "📑");
cmdBar->AddSecondary("文档属性", "ℹ️");
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
