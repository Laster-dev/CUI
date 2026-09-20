#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/TextBlock.h"

using namespace CUI;

using namespace CUI::DSL;

ShowcasePage BuildFilePickerPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref filePicker = Widgets::FilePicker().Width(360).Shared();
    filePicker->ClearFilters();
    filePicker->AddFilter("文本文件", "*.txt");
    filePicker->AddFilter("所有文件", "*.*");

    CUI::Widgets::Ref folderPicker = Widgets::FolderPicker().Width(360).Shared();

    CUI::Widgets::Ref pathLabel =std::static_pointer_cast<CUI::TextBlock>(CreateShowcaseText("未选择文件", 12.0f, "textMuted"));
    filePicker->OnPathChanged().Connect([pathLabel](CUI::FilePicker*, const std::string& path) {
        if (pathLabel) {
                        pathLabel.Text(path.empty() ? "未选择文件" : ("已选文件: " + path));
        }
    });

    CUI::Widgets::Ref folderLabel =std::static_pointer_cast<CUI::TextBlock>(CreateShowcaseText("未选择文件夹", 12.0f, "textMuted"));
    folderPicker->OnPathChanged().Connect([folderLabel](CUI::FolderPicker*, const std::string& path) {
        if (folderLabel) {
                        folderLabel.Text(path.empty() ? "未选择文件夹" : ("已选文件夹: " + path));
        }
    });

    return { "FilePicker 文件/文件夹", CreatePage(
        "FilePicker / FolderPicker",
        "自绘路径框 + 弹层文件浏览器。顶部 BreadcrumbBar，底部 TreeView 树形浏览；折叠后不残留绘制。",
        CreateDemoSurface({
            CreateShowcaseText("FilePicker — 树形浏览 + 面包屑跳转 + 文件类型下拉筛选", 12.0f, "textMuted"),
            filePicker,
            pathLabel,
            CreateShowcaseText("FolderPicker — 树形选择文件夹后点击「选择」确认", 12.0f, "textMuted"),
            folderPicker,
            folderLabel
        })) };
}
