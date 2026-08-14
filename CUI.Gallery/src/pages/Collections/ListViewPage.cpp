#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/ListView.h"

#include <memory>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {
namespace {

class DemoListViewDataSource final : public ListViewDataSource {
public:
    std::string GetCellText(int row, int column) override {
        switch (column) {
        case 0: return "文件_" + std::to_string(row + 1) + ".log";
        case 1: return row % 3 == 0 ? "日志" : (row % 3 == 1 ? "文本" : "数据");
        case 2: return std::to_string((row + 1) * 8) + " KB";
        default: return "";
        }
    }
};

std::vector<std::vector<std::string>> DemoRows() {
    return {
        { "README.md", "Markdown", "8 KB" },
        { "CUI.Core", "项目", "2.4 MB" },
        { "CUI.Gallery", "项目", "1.1 MB" },
        { "theme.json", "JSON", "14 KB" },
        { "notes.txt", "文本", "3 KB" },
    };
}

} // namespace

std::shared_ptr<UIElement> BuildListViewPage() {
    auto table = Make<ListView>();
    table->SetHeight(250.0f);
    table->SetWidth(620.0f);
    table->AddColumn("名称", 240.0f);
    table->AddColumn("类型", 150.0f);
    table->AddColumn("大小", 120.0f);
    table->SetRows(DemoRows());
    table->SetSelectionMode(ListViewSelectionMode::Extended);

    State<std::string> tableStatusText{ "单击表头可排序；拖动列分隔线可调整宽度。" };
    auto tableStatus = MakeStatus("");
    tableStatus->Text->Bind(tableStatusText, BindingMode::OneWay);
    table->OnSelectionChanged().Connect([table, tableStatusText](ListView*, int index) {
        const size_t count = table->GetSelectedIndices().size();
        tableStatusText = index < 0
            ? "当前没有选中行。"
            : "当前焦点行为第 " + std::to_string(index + 1) + " 行；已选 " + std::to_string(count) + " 行。";
    });
    table->OnRowDoubleClicked().Connect([tableStatusText](ListView*, int index) {
        tableStatusText = "双击第 " + std::to_string(index + 1) + " 行，可在业务中打开详情。";
    });
    table->OnColumnHeaderClicked().Connect([tableStatusText](ListView*, int column, bool ascending) {
        tableStatusText = "已按第 " + std::to_string(column + 1) + " 列" + (ascending ? "升序" : "降序") + "排列。";
    });

    State<int> appendedRows{ 1 };
    auto addRow = Make<Button>("追加一行");
    addRow->OnClick().Connect([table, appendedRows, tableStatusText](UIElement*) {
        const int serial = appendedRows.Get();
        appendedRows = serial + 1;
        table->AddRow({ "新增文件_" + std::to_string(serial) + ".txt", "文本", "1 KB" });
        tableStatusText = "已追加第 " + std::to_string(table->GetRowCount()) + " 行。";
    });
    auto resetRows = Make<Button>("重置行");
    resetRows->OnClick().Connect([table, tableStatusText](UIElement*) {
        table->SetRows(DemoRows());
        table->ClearSelection();
        tableStatusText = "已恢复内存中的演示行。";
    });
    auto clearRows = Make<Button>("清空行");
    clearRows->OnClick().Connect([table, tableStatusText](UIElement*) {
        table->ClearRows();
        tableStatusText = "已清空全部内存行。";
    });
    auto selectAll = Make<Button>("全选");
    selectAll->OnClick().Connect([table](UIElement*) { table->SelectAll(); });
    auto clearSelection = Make<Button>("清除选择");
    clearSelection->OnClick().Connect([table](UIElement*) { table->ClearSelection(); });
    auto compactRows = Make<Button>("紧凑行高");
    compactRows->OnClick().Connect([table, tableStatusText](UIElement*) {
        table->SetRowHeight(22.0f);
        tableStatusText = "行高已设为 22px。";
    });
    auto comfortableRows = Make<Button>("舒适行高");
    comfortableRows->OnClick().Connect([table, tableStatusText](UIElement*) {
        table->SetRowHeight(36.0f);
        tableStatusText = "行高已设为 36px。";
    });
    auto toggleGrid = Make<Button>("切换网格线");
    toggleGrid->OnClick().Connect([table](UIElement*) {
        table->SetShowGridLines(!table->GetShowGridLines());
    });
    auto toggleSizeColumn = Make<Button>("显示/隐藏大小列");
    toggleSizeColumn->OnClick().Connect([table](UIElement*) {
        table->SetColumnVisible(2, !table->IsColumnVisible(2));
    });

    auto customCells = Make<ListView>();
    customCells->SetHeight(140.0f);
    customCells->SetWidth(620.0f);
    customCells->AddColumn("任务", 280.0f);
    customCells->AddColumn("状态", 180.0f);
    auto ready = Make<TextBlock>("✓ 已完成");
    ready->SetTextColor(Color::Hex("#16803C"));
    auto pending = Make<TextBlock>("● 进行中");
    pending->SetTextColor(Color::Hex("#C26A00"));
    customCells->AddRow({ { "构建 CUI.Core", nullptr }, { "", ready } });
    customCells->AddRow({ { "完善集合控件示例", nullptr }, { "", pending } });

    static DemoListViewDataSource virtualSource;
    auto virtualTable = Make<ListView>();
    virtualTable->SetHeight(210.0f);
    virtualTable->SetWidth(620.0f);
    virtualTable->AddColumn("名称", 280.0f);
    virtualTable->AddColumn("类别", 150.0f);
    virtualTable->AddColumn("大小", 120.0f);
    virtualTable->SetVirtualMode(100000, &virtualSource);
    virtualTable->SetShowGridLines(false);
    State<std::string> virtualStatusText{ "虚拟表格包含 100,000 行；滚动时仅按需读取单元格文本。" };
    auto virtualStatus = MakeStatus("");
    virtualStatus->Text->Bind(virtualStatusText, BindingMode::OneWay);
    virtualTable->OnSelectionChanged().Connect([virtualStatusText](ListView*, int row) {
        virtualStatusText = row < 0 ? "未选择虚拟行。" : "已选择虚拟行 #" + std::to_string(row + 1) + "。";
    });
    auto reveal = Make<Button>("定位到第 50,000 行");
    reveal->OnClick().Connect([virtualTable](UIElement*) {
        virtualTable->SetCaretIndex(49999);
        virtualTable->SetRowSelected(49999, true);
        virtualTable->EnsureVisible(49999);
    });

    SamplePageSpec spec;
    spec.title = "ListView(列表视图)";
    spec.subtitle = "面向二维表格集合：列排序、调整列宽、行选择、批量操作、自定义单元格与高性能虚拟行。";
    spec.sections = {
        {
            "内存表格、列与行操作",
            "点击列标题排序；拖动标题边缘调整宽度；扩展选择支持 Ctrl、Shift 和 Ctrl+A。",
            Column(10).Children({
                table,
                tableStatus,
                Row(8).Children({ addRow, resetRows, clearRows, selectAll, clearSelection }).Build(),
                Row(8).Children({ compactRows, comfortableRows, toggleGrid, toggleSizeColumn }).Build(),
            }).Build(),
        },
        {
            "自定义单元格",
            "ListViewCellData 可在任意单元格放入 UIElement，适合状态徽标、进度条或操作按钮。",
            customCells,
        },
        {
            "虚拟行数据源",
            "ListViewDataSource 适用于十万级以上数据。控件不构建完整二维字符串或单元格控件。",
            Column(8).Children({ virtualTable, virtualStatus, reveal }).Build(),
        },
    };
    spec.source =
        "auto table = Make<ListView>();\n"
        "table->AddColumn(\"名称\", 240);\n"
        "table->AddColumn(\"类型\", 150);\n"
        "table->SetRows(rows);\n"
        "table->SetSelectionMode(ListViewSelectionMode::Extended);\n"
        "table->SetVirtualMode(100000, &dataSource);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
