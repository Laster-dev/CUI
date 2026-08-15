#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/ToggleButton.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 ToolTip (悬停提示) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽度。
 */
Element BuildToolTipPage() {
    auto statusLabel = MakeStatus("提示：将鼠标光标移动到下方按钮或文本上方稍作停留，即可观察浮层提示框。");

    // ==========================================
    // 1. 常用操作按钮工具提示
    // ==========================================
    auto btnSave = Button("💾 保存文件")
        .ToolTip("保存当前编辑的文件内容 (快捷键: Ctrl + S)")
        .OnClick([statusLabel](UIElement*) {
            statusLabel->Text = "已触发【保存文件】操作。";
        });

    auto btnUndo = Button("↩️ 撤销")
        .ToolTip("撤销上一步操作 (快捷键: Ctrl + Z)")
        .OnClick([statusLabel](UIElement*) {
            statusLabel->Text = "已触发【撤销】操作。";
        });

    auto btnRedo = Button("↪️ 重做")
        .ToolTip("恢复刚刚撤销的内容 (快捷键: Ctrl + Y)")
        .OnClick([statusLabel](UIElement*) {
            statusLabel->Text = "已触发【重做】操作。";
        });

    auto btnBuild = Button("⚡ 一键编译")
        .ToolTip("启动 MSVC C++20 模块化增量构建与静态代码分析检测")
        .OnClick([statusLabel](UIElement*) {
            statusLabel->Text = "已触发【一键编译】操作。";
        });

    // ==========================================
    // 2. 超长多行文本换行提示 (Multi-line ToolTip)
    // ==========================================
    auto btnMultiLine = Button("📄 查看核心渲染管线架构说明")
        .ToolTip("Direct2D / DirectWrite 硬件加速渲染管线说明：\n"
                 "1. 双重缓冲与亚像素抗锯齿排版；\n"
                 "2. 局部脏矩形剪裁与高精度图层缓存；\n"
                 "3. 60+ FPS 流畅动画与触控/滚轮物理缓动。")
        .OnClick([statusLabel](UIElement*) {
            statusLabel->Text = "悬停可查看带多行换行的详细架构描述。";
        });

    // ==========================================
    // 3. 多种控件的通用 ToolTip 挂载
    // ==========================================
    auto toggleAutoSave = ToggleButtonWidget("自动同步 (Auto Sync)");
    toggleAutoSave->SetToolTip("开启后，每当文档内容发生更改时将自动写入本地缓存文件");

    auto txtSample = TextField()
        .Text("鼠标悬停在输入框查看提示")
        .ToolTip("提示：这是一个单行文本输入框，支持快捷键全选与剪贴板操作");

    SamplePageSpec spec;
    spec.title = "ToolTip (悬停提示)";
    spec.subtitle = "为任意控件提供非侵入式的即时悬停上下文提示，支持单行快捷键提示、多行自动折行说明与智能屏幕边缘避让。";
    spec.sections = {
        {
            "基础快捷键与操作提示",
            "将鼠标悬停在各操作按钮上方，提示框将自动计算宿主边界并呈现带阴影的现代 Fluent 气泡。",
            Column(12, {
                Row(8, { btnSave, btnUndo, btnRedo, btnBuild }),
            }),
        },
        {
            "多行格式化长文本提示",
            "支持在 ToolTip 中嵌入多行文本与换行符，提示框自适应测量内容尺寸并进行平滑边缘避让。",
            Column(12, {
                btnMultiLine,
            }),
        },
        {
            "通用控件工具提示挂载",
            "CUI 的所有 UIElement 派生控件（如切换按钮、文本输入框等）均天然具备 ToolTip 属性。",
            Column(12, {
                Row(12, { toggleAutoSave, txtSample }),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 在链式 DSL 中直接配置 ToolTip
auto btn = Button("保存")
    .ToolTip("保存当前修改 (Ctrl + S)")
    .OnClick([](UIElement*) { /* ... */ });

// 2. 在任意 UIElement 实例上动态设置
element->SetToolTip("多行提示说明：\n- 第一点\n- 第二点");
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
