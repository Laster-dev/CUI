#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/LogView.h"
#include "framework/controls/Button.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 LogView (高频日志监视器) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽度。
 */
Element BuildLogViewPage() {
    auto statusLabel = MakeStatus("提示：LogView 支持多级别色彩高亮、快速关键字检索、双击行选中与尾部跟随 (Follow Tail)。");

    // ==========================================
    // 1. 初始化 LogView 实例
    // ==========================================
    auto logView = LogViewWidget()
        .Height(300.0f)
        .CornerRadius(6.0f)
        .Build();

    // 预填初始日志条目
    logView->Append(LogLevel::Info, "System", "CUI 现代桌面 UI 框架引擎初始化完成。");
    logView->Append(LogLevel::Debug, "D2D1", "已成功绑定 ID2D1DeviceContext6 硬件加速管线。");
    logView->Append(LogLevel::Trace, "FontCache", "DirectWrite 字体缓存池预热：微软雅黑 (12pt, 14pt, 16pt)。");
    logView->Append(LogLevel::Info, "Layout", "DockManager 主视口弹性排版编排完成，各象限尺寸已就绪。");
    logView->Append(LogLevel::Warn, "TextureCache", "纹理贴图缓存池占用达 64MB，触发 LRU 局部淘汰机制。");
    logView->Append(LogLevel::Error, "NetClient", "远程热重载服务连接失败 (ws://127.0.0.1:9092) - 连接被拒绝。");
    logView->Append(LogLevel::Debug, "Animation", "ChromiumScrollAnimator 启动平滑插值，步长: 16.6ms。");
    logView->Append(LogLevel::Info, "Build", "增量编译完成，输出目标: CUI.Gallery.exe (0 errors, 0 warnings)。");

    // ==========================================
    // 2. 交互操作按钮
    // ==========================================
    static int s_logSeq = 1;

    auto btnAddInfo = Button("ℹ️ 注入常规日志 (Info)")
        .OnClick([logView, statusLabel](UIElement*) {
            logView->Append(LogLevel::Info, "Worker", std::format("后台任务 #{} 批处理完成，耗时: 12.4ms", s_logSeq++));
            statusLabel->Text = "已追加一条【Info 级别】日志。";
        });

    auto btnAddWarn = Button("⚠️ 注入警告日志 (Warn)")
        .OnClick([logView, statusLabel](UIElement*) {
            logView->Append(LogLevel::Warn, "Memory", std::format("内存碎片率较高 ({}%)，建议触发 GC", 60 + (s_logSeq % 30)));
            statusLabel->Text = "已追加一条【Warn 级别】日志。";
        });

    auto btnAddError = Button("❌ 注入错误日志 (Error)")
        .OnClick([logView, statusLabel](UIElement*) {
            logView->Append(LogLevel::Error, "Shader", "HLSL 像素着色器编译失败: 语法错误位于第 48 行。");
            statusLabel->Text = "已追加一条【Error 级别】日志。";
        });

    auto btnAddFatal = Button("💀 致命错误 (Fatal)")
        .OnClick([logView, statusLabel](UIElement*) {
            logView->Append(LogLevel::Fatal, "Kernel", "硬件断言触发: ACCESS_VIOLATION at 0x00007FF789A012");
            statusLabel->Text = "已追加一条【Fatal 级别】日志。";
        });

    auto btnClear = Button("🗑️ 清空日志 (Clear)")
        .OnClick([logView, statusLabel](UIElement*) {
            logView->Clear();
            statusLabel->Text = "已清空当前所有日志流记录。";
        });

    auto btnCopy = Button("📋 复制全部可见日志")
        .OnClick([logView, statusLabel](UIElement*) {
            logView->SelectAllVisible();
            logView->CopyVisible();
            statusLabel->Text = "已复制当前全部可见日志至系统剪贴板。";
        });

    SamplePageSpec spec;
    spec.title = "LogView (日志视图)";
    spec.subtitle = "高性能实时日志流监视器，内置多级别着色 (Trace/Debug/Info/Warn/Error/Fatal)、关键字搜索过滤、自动滚底跟随与剪贴板复制。";
    spec.sections = {
        {
            "高频滚动实时日志看板",
            "自适应填充视口宽度，支持快捷键 (Ctrl+A / Ctrl+C)、双击快速选中单行以及右侧平滑滚动条。",
            Column(12, {
                logView,
            }),
        },
        {
            "动态日志流模拟注入",
            "点击下方各按钮向 LogView 高频写入不同级别与分类标签的日志条目，测试流式滚动与颜色分级。",
            Column(12, {
                Row(8, { btnAddInfo, btnAddWarn, btnAddError, btnAddFatal }),
                Row(8, { btnClear, btnCopy }),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建 LogView 实例
auto logView = LogViewWidget()
    .Height(300.0f)
    .Build();

// 2. 追加各级别分类日志条目
logView->Append(LogLevel::Info, "System", "引擎初始化完成");
logView->Append(LogLevel::Warn, "Memory", "内存碎片预警");
logView->Append(LogLevel::Error, "Net", "连接重置");

// 3. 过滤与控制
logView->SetFilterText("Error"); // 关键字筛选
logView->SetFollowTail(true);    // 始终锁定滚底跟随
logView->Clear();                // 清空记录
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
