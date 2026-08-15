#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/StatusBar.h"
#include "framework/controls/Button.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 StatusBar (状态栏) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽度。
 */
Element BuildStatusBarPage() {
    auto statusLabel = MakeStatus("提示：状态栏支持左侧自适应伸展、右侧固定/进度对齐与分割线。");

    // ==========================================
    // 1. 经典 IDE 状态栏实例
    // ==========================================
    auto bar = StatusBarWidget()
        .Height(28.0f)
        .Background(D2D1::ColorF(0x18181B, 0.9f))
        .Border(D2D1::ColorF(0x27272A, 1.0f), 1.0f)
        .CornerRadius(4.0f)
        .Build();

    int idStatus = bar->AddTextItem("就绪 (Ready)", StatusBarItemAlignment::Left);
    bar->AddSeparator(StatusBarItemAlignment::Left);
    int idBranch = bar->AddTextItem("🌿 main*", StatusBarItemAlignment::Left);
    bar->AddSeparator(StatusBarItemAlignment::Left);
    int idChanges = bar->AddTextItem("0 错误, 0 警告", StatusBarItemAlignment::Left);

    int idProgress = bar->AddProgressItem("后台索引中...", StatusBarItemAlignment::Right, 130.0f);
    bar->SetItemProgress(idProgress, 0.65f);
    bar->AddSeparator(StatusBarItemAlignment::Right);
    int idPos = bar->AddTextItem("Ln 128, Col 32", StatusBarItemAlignment::Right, 100.0f);
    bar->AddSeparator(StatusBarItemAlignment::Right);
    int idEncoding = bar->AddTextItem("UTF-8", StatusBarItemAlignment::Right, 60.0f);
    bar->AddSeparator(StatusBarItemAlignment::Right);
    int idCrLf = bar->AddTextItem("CRLF", StatusBarItemAlignment::Right, 50.0f);

    // ==========================================
    // 2. 交互控制按钮
    // ==========================================
    static float s_prog = 0.65f;
    auto btnProgress = Button("更新后台进度条")
        .OnClick([bar, idProgress, statusLabel](UIElement*) {
            s_prog += 0.20f;
            if (s_prog > 1.05f) {
                s_prog = 0.0f;
                bar->SetItemText(idProgress, "就绪");
                bar->SetItemProgress(idProgress, -1.0f);
                statusLabel->Text = "后台任务已完成，进度条隐藏。";
            } else {
                bar->SetItemText(idProgress, std::format("构建进度 {:.0f}%", s_prog * 100.0f));
                bar->SetItemProgress(idProgress, s_prog);
                statusLabel->Text = std::format("已更新后台进度为 {:.0f}%", s_prog * 100.0f);
            }
        });

    static bool s_branchToggle = false;
    auto btnBranch = Button("切换 Git 分支显示")
        .OnClick([bar, idBranch, statusLabel](UIElement*) {
            s_branchToggle = !s_branchToggle;
            if (s_branchToggle) {
                bar->SetItemText(idBranch, "🔀 feature/fluent-v2");
                statusLabel->Text = "状态栏已切换为特性分支：【feature/fluent-v2】";
            } else {
                bar->SetItemText(idBranch, "🌿 main*");
                statusLabel->Text = "状态栏已切回主干分支：【main*】";
            }
        });

    static int s_lineNum = 128;
    auto btnCursor = Button("模拟光标移动")
        .OnClick([bar, idPos, statusLabel](UIElement*) {
            s_lineNum += 15;
            bar->SetItemText(idPos, std::format("Ln {}, Col 12", s_lineNum));
            statusLabel->Text = std::format("已更新当前编辑光标坐标：Ln {}, Col 12", s_lineNum);
        });

    SamplePageSpec spec;
    spec.title = "StatusBar (状态栏)";
    spec.subtitle = "桌面应用底部常驻信息状态条，支持左/中/右多区段对齐、实时进度指示器、动态图文与高精度排版。";
    spec.sections = {
        {
            "IDE 风格弹性底部状态栏",
            "状态栏自适应填充可用行宽，左侧承载状态与分支信息，右侧承载高频刷新的进度条、行列号与编码指示器。",
            Column(12, {
                bar,
            }),
        },
        {
            "动态状态与进度修改",
            "点击下方按钮动态修改状态栏子项的文本、图标以及进度条填充比例。",
            Column(12, {
                Row(8, { btnProgress, btnBranch, btnCursor }),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建状态栏控件
auto bar = StatusBarWidget()
    .Height(28.0f)
    .Build();

// 2. 添加左侧状态与分支
int idStatus = bar->AddTextItem("就绪", StatusBarItemAlignment::Left);
int idBranch = bar->AddTextItem("🌿 main*", StatusBarItemAlignment::Left);

// 3. 添加右侧进度条与编码
int idProgress = bar->AddProgressItem("同步中...", StatusBarItemAlignment::Right, 120.0f);
bar->SetItemProgress(idProgress, 0.75f);
int idEncoding = bar->AddTextItem("UTF-8", StatusBarItemAlignment::Right, 60.0f);
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
