#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/TextBlock.h"

using namespace CUI::DSL;

ShowcasePage BuildFilePickerPage(const ShowcaseContext& ctx) {
    auto filePicker = FilePickerWidget().Width(360).Build();
    filePicker->ClearFilters();
    filePicker->AddFilter("文本文件", "*.txt");
    filePicker->AddFilter("所有文件", "*.*");

    auto folderPicker = FolderPickerWidget().Width(360).Build();

    auto pathLabel = std::static_pointer_cast<CUI::TextBlock>(CreateShowcaseText("未选择文件", 12.0f, "textMuted"));
    filePicker->OnPathChanged().Connect([pathLabel](CUI::FilePicker*, const std::string& path) {
        if (pathLabel) {
            pathLabel->SetText(path.empty() ? "未选择文件" : ("已选文件: " + path));
        }
    });

    auto folderLabel = std::static_pointer_cast<CUI::TextBlock>(CreateShowcaseText("未选择文件夹", 12.0f, "textMuted"));
    folderPicker->OnPathChanged().Connect([folderLabel](CUI::FolderPicker*, const std::string& path) {
        if (folderLabel) {
            folderLabel->SetText(path.empty() ? "未选择文件夹" : ("已选文件夹: " + path));
        }
    });

    return { "FilePicker 文件/文件夹", CreatePage(
        "FilePicker / FolderPicker",
        "自绘路径框 + 浏览按钮，调用系统 IFileOpenDialog 选择文件或文件夹。",
        CreateDemoSurface({
            CreateShowcaseText("FilePicker — 点击路径或 ▼ 按钮打开文件对话框", 12.0f, "textMuted"),
            filePicker,
            pathLabel,
            CreateShowcaseText("FolderPicker — 选择文件夹", 12.0f, "textMuted"),
            folderPicker,
            folderLabel
        }),
        CreatePropertyGrid(ctx, filePicker), filePicker) };
}
