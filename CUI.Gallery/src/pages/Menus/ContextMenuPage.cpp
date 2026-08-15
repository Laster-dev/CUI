#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/ContextMenu.h"
#include "framework/controls/TextBox.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 ContextMenu (上下文右键菜单) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽高。
 */
Element BuildContextMenuPage() {
    auto statusLabel = MakeStatus("在下方任意目标工作区内【单击鼠标右键】，即可呼出上下文右键菜单。");

    // ==========================================
    // 示例 1: 文件工作区右键菜单
    // ==========================================
    auto workspaceMenu = std::make_shared<ContextMenu>();

    // 1. 新建子菜单 (SubMenu)
    auto newSubMenu = workspaceMenu->AddSubMenu("新建 (New)");
    newSubMenu->AddItem("文本文档 (.txt)", [statusLabel]() {
        statusLabel->Text = "已从右键菜单创建：【新建文本文档.txt】";
    });
    newSubMenu->AddItem("C++ 源代码 (.cpp)", [statusLabel]() {
        statusLabel->Text = "已从右键菜单创建：【main.cpp】";
    });
    newSubMenu->AddItem("JSON 配置文件 (.json)", [statusLabel]() {
        statusLabel->Text = "已从右键菜单创建：【config.json】";
    });
    newSubMenu->AddItem("Markdown 笔记 (.md)", [statusLabel]() {
        statusLabel->Text = "已从右键菜单创建：【README.md】";
    });

    // 2. 查看与排序子菜单
    auto viewSubMenu = workspaceMenu->AddSubMenu("查看方式 (View)");
    viewSubMenu->AddItem("大图标", [statusLabel]() {
        statusLabel->Text = "视图模式已切换为：【大图标 (Large Icons)】";
    });
    auto itemDetailList = viewSubMenu->AddItem("详细列表", [statusLabel]() {
        statusLabel->Text = "视图模式已切换为：【详细信息列表 (Details)】";
    });
    itemDetailList->SetChecked(true);

    workspaceMenu->AddSeparator();

    // 3. 基础剪贴板操作
    workspaceMenu->AddItem("剪切 (Cut)", "Ctrl+X", [statusLabel]() {
        statusLabel->Text = "已执行右键操作：【剪切】至系统剪贴板";
    });
    workspaceMenu->AddItem("复制 (Copy)", "Ctrl+C", [statusLabel]() {
        statusLabel->Text = "已执行右键操作：【复制】至系统剪贴板";
    });
    workspaceMenu->AddItem("粘贴 (Paste)", "Ctrl+V", [statusLabel]() {
        statusLabel->Text = "已执行右键操作：【粘贴】剪贴板数据";
    });
    workspaceMenu->AddItem("重命名 (Rename)", "F2", [statusLabel]() {
        statusLabel->Text = "已进入【重命名】编辑状态";
    });
    workspaceMenu->AddItem("删除 (Delete)", "Del", [statusLabel]() {
        statusLabel->Text = "已将选中项移动至【回收站】";
    });

    workspaceMenu->AddSeparator();

    // 4. 终端与属性
    workspaceMenu->AddItem("在集成终端中打开", "Ctrl+`", [statusLabel]() {
        statusLabel->Text = "已在当前目录启动【PowerShell 终端实例】";
    });
    workspaceMenu->AddItem("工作区属性", "Alt+Enter", [statusLabel]() {
        statusLabel->Text = "已打开【工作区文件夹属性】对话框";
    });

    // 承载右键菜单的文件管理工作区卡片 (自适应撑满容器宽度，高度 120px)
    auto wsTitle = Text("📁 项目资源工作区 (右键点击此卡片区域)")
        .FontSize(14.0f)
        .FontWeight(FontWeight::SemiBold);

    auto wsHint = Text("支持右键展开多级级联子菜单（新建、查看）、快捷键提示、勾选状态项以及分隔线。")
        .FontSize(12.0f)
        .Foreground(D2D1::ColorF(0x94A3B8, 1.0f));

    auto workspaceArea = Column(6, { wsTitle, wsHint })
        .Height(120.0f)
        .Background(D2D1::ColorF(0x202024, 0.4f))
        .Border(D2D1::ColorF(0x3B82F6, 0.4f), 1.5f)
        .CornerRadius(8.0f)
        .Padding(16.0f);

    workspaceArea->SetContextMenu(workspaceMenu);

    // ==========================================
    // 示例 2: 代码编辑器专属右键菜单
    // ==========================================
    auto codeMenu = std::make_shared<ContextMenu>();
    codeMenu->AddItem("格式化文档", "Shift+Alt+F", [statusLabel]() {
        statusLabel->Text = "编辑器：【代码格式化完成】(符合 Clang-Format 标准)";
    });
    codeMenu->AddItem("转到定义", "F12", [statusLabel]() {
        statusLabel->Text = "编辑器：【正在定位符号声明位置...】";
    });
    codeMenu->AddItem("查找所有引用", "Shift+F12", [statusLabel]() {
        statusLabel->Text = "编辑器：【检索到 14 处符号引用实例】";
    });
    codeMenu->AddSeparator();
    auto refactorSub = codeMenu->AddSubMenu("重构 (Refactor)");
    refactorSub->AddItem("提取为方法/函数", "Ctrl+R Ctrl+M", [statusLabel]() {
        statusLabel->Text = "重构：【已生成新函数签名并提取选中文本】";
    });
    refactorSub->AddItem("引入局部变量", "Ctrl+R Ctrl+V", [statusLabel]() {
        statusLabel->Text = "重构：【已提取表达式为局部常量变量】";
    });
    codeMenu->AddItem("快速修复与重命名", "Ctrl+.", [statusLabel]() {
        statusLabel->Text = "编辑器：【呼出智能修复与重命名提示条】";
    });

    auto codeEditorArea = TextField()
        .Text("// 在此代码编辑区内右键点击，呼出代码开发专属快捷菜单\n"
              "int main(int argc, char* argv[]) {\n"
              "    auto app = CUI::Application::Create();\n"
              "    return app->Run();\n"
              "}")
        .Height(110.0f);
    codeEditorArea->SetAcceptsReturn(true);
    codeEditorArea->SetTextWrapping(true);
    codeEditorArea->SetContextMenu(codeMenu);

    SamplePageSpec spec;
    spec.title = "ContextMenu (上下文右键菜单)";
    spec.subtitle = "挂载于任意 UI 元素的浮层上下文弹出菜单，支持嵌套级联子菜单、快捷键提示符、勾选项与分隔线。";
    spec.sections = {
        {
            "工作区与文件管理右键菜单 (多级级联 · 快捷键 · 勾选态)",
            "在下方卡片区域内点击鼠标右键呼出菜单。支持平滑入场动画、阴影浮层以及键盘方向键导航。",
            Column(12, {
                workspaceArea,
            }),
        },
        {
            "文本与代码编辑专属右键菜单 (Code Editor Actions)",
            "为文本框或代码编辑器挂载定制的上下文动作菜单，提供精准的代码重构与导航功能。",
            Column(12, {
                codeEditorArea,
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建右键菜单并添加操作项与快捷键
auto menu = std::make_shared<ContextMenu>();

// 2. 添加多级级联子菜单
auto subMenu = menu->AddSubMenu("新建 (New)");
subMenu->AddItem("文本文档 (.txt)", []() { /* ... */ });
subMenu->AddItem("C++ 源代码 (.cpp)", []() { /* ... */ });

menu->AddSeparator();
menu->AddItem("剪切", "Ctrl+X", []() { /* ... */ });
menu->AddItem("复制", "Ctrl+C", []() { /* ... */ });

// 3. 将右键菜单挂载至任意目标控件
targetElement->SetContextMenu(menu);
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
