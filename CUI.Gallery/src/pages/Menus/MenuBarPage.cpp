#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/MenuBar.h"
#include "framework/controls/TextBox.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 MenuBar (顶级菜单栏) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽高。
 */
Element BuildMenuBarPage() {
    auto statusLabel = MakeStatus("点击菜单栏顶部的各菜单项（或按 Alt+对应助记键），即可下拉展开桌面级功能菜单。");

    // ==========================================
    // 示例 1: 桌面应用全功能菜单栏 (Full Application MenuBar)
    // ==========================================
    auto appMenuBar = MenuBarWidget().Build();

    // 1. 文件 (File) 菜单
    auto fileMenu = appMenuBar->AddMenu("文件 (F)");
    fileMenu->AddItem("新建文件", "Ctrl+N", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【文件】 -> 【新建文件 (Ctrl+N)】";
    });
    fileMenu->AddItem("打开文件...", "Ctrl+O", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【文件】 -> 【打开文件 (Ctrl+O)】";
    });
    fileMenu->AddItem("打开文件夹...", "Ctrl+K Ctrl+O", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【文件】 -> 【打开文件夹】";
    });
    fileMenu->AddSeparator();
    fileMenu->AddItem("保存", "Ctrl+S", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【文件】 -> 【保存 (Ctrl+S)】";
    });
    fileMenu->AddItem("另存为...", "Ctrl+Shift+S", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【文件】 -> 【另存为...】";
    });
    fileMenu->AddSeparator();
    auto prefSub = fileMenu->AddSubMenu("首选项 (P)");
    prefSub->AddItem("应用全局设置", "Ctrl+,", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【文件】 -> 【首选项】 -> 【设置 (Ctrl+,)】";
    });
    prefSub->AddItem("快捷键映射表", "Ctrl+K Ctrl+S", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【文件】 -> 【首选项】 -> 【快捷键映射】";
    });
    prefSub->AddItem("主题色彩管理", "Ctrl+K Ctrl+T", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【文件】 -> 【首选项】 -> 【主题色彩】";
    });
    fileMenu->AddSeparator();
    fileMenu->AddItem("退出 (X)", "Alt+F4", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【文件】 -> 【退出系统 (Alt+F4)】";
    });

    // 2. 编辑 (Edit) 菜单
    auto editMenu = appMenuBar->AddMenu("编辑 (E)");
    editMenu->AddItem("撤销", "Ctrl+Z", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【编辑】 -> 【撤销 (Ctrl+Z)】";
    });
    editMenu->AddItem("重做", "Ctrl+Y", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【编辑】 -> 【重做 (Ctrl+Y)】";
    });
    editMenu->AddSeparator();
    editMenu->AddItem("剪切", "Ctrl+X", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【编辑】 -> 【剪切 (Ctrl+X)】";
    });
    editMenu->AddItem("复制", "Ctrl+C", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【编辑】 -> 【复制 (Ctrl+C)】";
    });
    editMenu->AddItem("粘贴", "Ctrl+V", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【编辑】 -> 【粘贴 (Ctrl+V)】";
    });
    editMenu->AddSeparator();
    editMenu->AddItem("查找", "Ctrl+F", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【编辑】 -> 【查找 (Ctrl+F)】";
    });
    editMenu->AddItem("替换", "Ctrl+H", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【编辑】 -> 【替换 (Ctrl+H)】";
    });

    // 3. 视图 (View) 菜单
    auto viewMenu = appMenuBar->AddMenu("视图 (V)");
    auto appSub = viewMenu->AddSubMenu("外观与排版");
    auto itemWrap = appSub->AddItem("自动换行 (Word Wrap)", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【视图】 -> 【自动换行状态已切换】";
    });
    itemWrap->SetChecked(true);
    appSub->AddItem("全屏模式", "F11", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【视图】 -> 【全屏展示 (F11)】";
    });
    appSub->AddItem("专注禅模式", "Ctrl+K Z", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【视图】 -> 【专注禅模式】";
    });

    auto zoomSub = viewMenu->AddSubMenu("缩放等级");
    zoomSub->AddItem("放大 (+)", "Ctrl+=", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【视图】 -> 【缩放: 放大】";
    });
    zoomSub->AddItem("缩小 (-)", "Ctrl+-", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【视图】 -> 【缩放: 缩小】";
    });
    zoomSub->AddItem("重置缩放 (100%)", "Ctrl+0", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【视图】 -> 【缩放: 重置为 100%】";
    });

    viewMenu->AddSeparator();
    viewMenu->AddItem("输出面板", "Ctrl+Shift+U", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【视图】 -> 【打开输出日志面板】";
    });
    viewMenu->AddItem("集成终端", "Ctrl+`", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【视图】 -> 【呼出底部集成终端】";
    });

    // 4. 帮助 (Help) 菜单
    auto helpMenu = appMenuBar->AddMenu("帮助 (H)");
    helpMenu->AddItem("快速入门指南", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【帮助】 -> 【打开快速上手入门指南】";
    });
    helpMenu->AddItem("快捷键参考清单", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【帮助】 -> 【打开常用键盘快捷键速查表】";
    });
    helpMenu->AddSeparator();
    helpMenu->AddItem("检查更新...", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【帮助】 -> 【当前已是最新 CUI 框架发行版本】";
    });
    helpMenu->AddItem("关于 CUI 框架", [statusLabel]() {
        statusLabel->Text = "菜单栏指令：【帮助】 -> 【CUI Modern Desktop Framework v2.0】";
    });

    // 模拟集成应用窗口工作区 (自适应撑满容器)
    auto mockContent = TextField()
        .Text("这是一个集成了顶级 MenuBar 的桌面应用工作区容器。\n"
              "MenuBar 占据顶端并随窗口宽度自动拉伸，支持鼠标悬停平滑高亮切换、点击展开级联下拉、助记键聚焦与快捷键响应。")
        .Height(120.0f)
        .Margin(12.0f);
    mockContent->SetAcceptsReturn(true);
    mockContent->SetTextWrapping(true);

    auto mockAppContainer = Column(0, { appMenuBar, mockContent })
        .Background(D2D1::ColorF(0x18181B, 0.6f))
        .Border(D2D1::ColorF(0x3F3F46, 0.5f), 1.0f)
        .CornerRadius(8.0f);

    SamplePageSpec spec;
    spec.title = "MenuBar (菜单栏)";
    spec.subtitle = "水平顶级应用菜单栏，支持多级级联子菜单、Alt 助记键导航、快捷键提示与响应式浮层展示。";
    spec.sections = {
        {
            "经典桌面应用顶级菜单栏 (File / Edit / View / Help)",
            "点击菜单标题（文件、编辑、视图、帮助）展开对应下拉菜单。支持鼠标在菜单栏各顶项间移动时自动平滑切换当前展开的菜单项。",
            Column(12, {
                mockAppContainer,
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建顶级菜单栏
auto menuBar = MenuBarWidget().Build();

// 2. 添加一级菜单与动作项
auto fileMenu = menuBar->AddMenu("文件 (F)");
fileMenu->AddItem("新建文件", "Ctrl+N", []() { /* ... */ });
fileMenu->AddItem("打开文件...", "Ctrl+O", []() { /* ... */ });
fileMenu->AddSeparator();

// 3. 添加多级级联子菜单
auto prefSub = fileMenu->AddSubMenu("首选项");
prefSub->AddItem("全局设置", "Ctrl+,", []() { /* ... */ });

// 4. 将菜单栏置于窗口或页面顶部
parentContainer->AddChild(menuBar);
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
