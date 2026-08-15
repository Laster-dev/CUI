#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/FilePicker.h"
#include "framework/controls/Button.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 FilePicker (文件选择器) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽度。
 */
Element BuildFilePickerPage() {
    auto statusLabel = MakeStatus("提示：点击输入框右侧的 📁 浏览按钮可直接呼出内置的非侵入式弹层文件选择器。");

    // ==========================================
    // 1. 基础文件选择器
    // ==========================================
    auto picker1 = FilePickerWidget("E:\\C++project\\CUI\\CUI.Core\\ui\\framework\\core\\CUIDsl.h")
        .Build();
    picker1->SetDialogTitle("选择目标代码源文件");
    picker1->SetFilter("C++ 源码文件 (*.cpp;*.h;*.hpp)", "*.cpp;*.h;*.hpp");
    picker1->AddFilter("JSON / 配置文件 (*.json;*.xml;*.yaml)", "*.json;*.xml;*.yaml");
    picker1->AddFilter("所有文件 (*.*)", "*.*");

    picker1->OnPathChanged().Connect([statusLabel](FilePicker*, const std::string& path) {
        statusLabel->Text = std::format("已选择文件路径：【{}】", path);
    });

    // ==========================================
    // 2. 交互控制预设切换
    // ==========================================
    auto btnCppFilter = Button("切换为 C++ 源码过滤")
        .OnClick([picker1, statusLabel](UIElement*) {
            picker1->SetFilter("C++ 源码 (*.cpp;*.h)", "*.cpp;*.h");
            picker1->SetPath("E:\\C++project\\CUI\\CUI.Core\\ui\\framework\\controls\\Button.cpp");
            statusLabel->Text = "已应用【C++ 源码】类型过滤器 (*.cpp;*.h)";
        });

    auto btnMediaFilter = Button("切换为图像资源过滤")
        .OnClick([picker1, statusLabel](UIElement*) {
            picker1->SetFilter("图像资源 (*.png;*.jpg;*.svg;*.ico)", "*.png;*.jpg;*.svg;*.ico");
            picker1->SetPath("E:\\C++project\\CUI\\assets\\icons\\app_logo.png");
            statusLabel->Text = "已应用【图像资源】类型过滤器 (*.png;*.jpg;*.svg)";
        });

    auto btnDocFilter = Button("切换为 Markdown 文档过滤")
        .OnClick([picker1, statusLabel](UIElement*) {
            picker1->SetFilter("Markdown 说明文档 (*.md;*.txt)", "*.md;*.txt");
            picker1->SetPath("E:\\C++project\\CUI\\README.md");
            statusLabel->Text = "已应用【文档】类型过滤器 (*.md;*.txt)";
        });

    SamplePageSpec spec;
    spec.title = "FilePicker (文件选择器)";
    spec.subtitle = "现代化文件拾取复合控件，集成单行路径显示、直接键盘输入编辑、文件类型过滤器与内置弹层目录树浏览窗。";
    spec.sections = {
        {
            "基础文件选择与弹层浏览器",
            "点击输入框右侧的浏览图标即可弹开无边框现代化文件选择器；选择文件后路径将自动回填并分发变更事件。",
            Column(12, {
                picker1,
            }),
        },
        {
            "类型过滤器与快速切换",
            "点击下方预设按钮，动态更新文件选择器支持的文件类型扩展名过滤规则以及默认预设路径。",
            Column(12, {
                Row(8, { btnCppFilter, btnMediaFilter, btnDocFilter }),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建 FilePicker 实例并配置初始路径
auto picker = FilePickerWidget("C:\\Projects\\App\\main.cpp")
    .Build();

// 2. 配置对话框标题与文件类型扩展名过滤器
picker->SetDialogTitle("选择 C++ 源文件");
picker->SetFilter("C++ 文件 (*.cpp;*.h)", "*.cpp;*.h");
picker->AddFilter("所有文件 (*.*)", "*.*");

// 3. 监听文件选择完成事件
picker->OnPathChanged().Connect([](FilePicker*, const std::string& path) {
    // 处理选中的文件绝对路径
});
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
