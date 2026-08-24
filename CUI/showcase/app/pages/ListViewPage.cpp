#include "framework/core/CUIDsl.h"
#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"

namespace {
class VirtualListViewSource : public CUI::ListViewDataSource {
public:
    std::string GetCellText(int row, int col) override {
        if (col == 0) return std::to_string(row + 1);
        if (col == 1) return "Item_Data_Row_" + std::to_string(row);
        if (col == 2) return (row % 3 == 0) ? "Running" : "Ready";
        return std::to_string((row * 7) % 100) + "%";
    }
};
}

ShowcasePage BuildListViewPage(const ShowcaseContext& ctx) {
    static VirtualListViewSource ds;
    auto target = std::make_shared<CUI::ListView>();
    CUI::DSL::Borrow(target).Width(500.0f);
    CUI::DSL::Borrow(target).Height(280.0f);
    target->AddColumn("ID", 60.0f);
    target->AddColumn("名称", 220.0f);
    target->AddColumn("状态", 120.0f);
    target->AddColumn("进度", 100.0f);
    CUI::DSL::Borrow(target).VirtualMode(100000, &ds);
    return { "ListView (100k)", CreatePage(
        "ListView 100k 多列虚拟化表单全属性控制台",
        "ListView 可滚动列表。",
        CreateDemoSurface({ target }, 0.0f)) };
}
