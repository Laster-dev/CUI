#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/FolderPicker.h"
#include "framework/controls/Button.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 FolderPicker (文件夹选择器) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽度。
 */
Element BuildFolderPickerPage() {
    auto statusLabel = MakeStatus("提示：点击输入框右侧的 📂 浏览按钮可直接呼出内置的目录树选择弹窗。");

    // ==========================================
    // 1. 基础文件夹目录选择器
    // ==========================================
    auto folderPicker = FolderPickerWidget("E:\\C++project\\CUI\\CUI.Gallery")
        .Build();
    folderPicker->SetDialogTitle("选择工程根目录");

    folderPicker->OnPathChanged().Connect([statusLabel](FolderPicker*, const std::string& path) {
        statusLabel->Text = std::format("已选择目标文件夹路径：【{}】", path);
    });

    // ==========================================
    // 2. 交互控制预设切换
    // ==========================================
    auto btnCoreDir = Button("选择 CUI.Core 框架源码目录")
        .OnClick([folderPicker, statusLabel](UIElement*) {
            folderPicker->SetPath("E:\\C++project\\CUI\\CUI.Core\\ui\\framework");
            statusLabel->Text = "已重定向目标目录至：【CUI.Core 框架源码】";
        });

    auto btnBuildDir = Button("选择构建输出目录 (Build Artifacts)")
        .OnClick([folderPicker, statusLabel](UIElement*) {
            folderPicker->SetPath("E:\\C++project\\CUI\\x64\\Debug");
            statusLabel->Text = "已重定向目标目录至：【构建输出目录 (x64\\Debug)】";
        });

    auto btnAssetsDir = Button("选择素材资源目录 (Assets)")
        .OnClick([folderPicker, statusLabel](UIElement*) {
            folderPicker->SetPath("E:\\C++project\\CUI\\CUI.Gallery\\assets");
            statusLabel->Text = "已重定向目标目录至：【素材资源目录 (assets)】";
        });

    SamplePageSpec spec;
    spec.title = "FolderPicker (文件夹选择器)";
    spec.subtitle = "现代化目录拾取控件，提供路径输入回显、快捷键盘编辑、内置面包屑导航栏以及层级目录树可视化选择弹窗。";
    spec.sections = {
        {
            "基础文件夹选择与目录树弹窗",
            "点击输入框右侧的文件夹图标即可展开目录树弹层；选择文件夹后路径将自动回显在输入栏中。",
            Column(12, {
                folderPicker,
            }),
        },
        {
            "常用工作区路径预设",
            "点击下方预设按钮可快速切换不同的工程目录或输出文件夹路径。",
            Column(12, {
                Row(8, { btnCoreDir, btnBuildDir, btnAssetsDir }),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建 FolderPicker 实例并设定初始目录
auto folderPicker = FolderPickerWidget("C:\\Projects\\Workspace")
    .Build();

// 2. 设置弹窗标题
folderPicker->SetDialogTitle("选择安装目标目录");

// 3. 监听目录路径更改事件
folderPicker->OnPathChanged().Connect([](FolderPicker*, const std::string& path) {
    // 处理选中的文件夹绝对路径
});
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
