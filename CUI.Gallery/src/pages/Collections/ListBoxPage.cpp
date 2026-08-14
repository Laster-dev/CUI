#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/ListBox.h"
#include "framework/controls/TextBox.h"

#include <memory>
#include <string>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {
namespace {

class DemoListBoxDataSource final : public ListBox::ListBoxDataSource {
public:
    std::string GetItemText(size_t index) override {
        return "虚拟记录 #" + std::to_string(index + 1);
    }
};

std::string SelectionSummary(ListBox* list, int index, const std::string& text) {
    const size_t count = list->GetSelectedIndices().size();
    if (index < 0) return "当前没有选中项。";
    return "选中第 " + std::to_string(index + 1) + " 项：" + text
        + "；已选 " + std::to_string(count) + " 项。";
}

} // namespace

std::shared_ptr<UIElement> BuildListBoxPage() {
    auto list = Make<ListBox>();
    list->SetHeight(220.0f);
    list->SetWidth(360.0f);
    list->SetSelectionMode(ListBoxSelectionMode::Extended);
    list->SetAllowDrag(true);
    list->SetAllowDrop(true);
    list->SetItems({ "收件箱", "今天", "本周", "已完成", "已归档", "垃圾箱" });

    State<std::string> selectionText{ "选择一个项目；扩展模式支持 Ctrl / Shift 和 Ctrl+A。" };
    auto selectionStatus = MakeStatus("");
    selectionStatus->Text->Bind(selectionText, BindingMode::OneWay);
    list->OnSelectionChanged().Connect([selectionText](ListBox* sender, int index, const std::string& text) {
        selectionText = SelectionSummary(sender, index, text);
    });
    list->OnItemDoubleClicked().Connect([selectionText](ListBox*, int index, const std::string& text) {
        selectionText = "双击第 " + std::to_string(index + 1) + " 项：" + text + "。";
    });

    auto input = Make<TextBox>();
    input->SetPlaceholder("输入新项目名称");
    input->SetWidth(220.0f);
	input->SetHeight(28.0f);
    State<int> generated{ 1 };

    auto add = Make<Button>("追加");
    add->OnClick().Connect([list, input, generated, selectionText](UIElement*) {
        std::string text = input->GetText();
        if (text.empty()) text = "新项目 " + std::to_string(generated.Get());
        generated = generated.Get() + 1;
        list->AddItem(text);
        input->Text->Set("");
        selectionText = "已在末尾追加：" + text + "。";
    });
    auto insert = Make<Button>("插入首项");
    insert->OnClick().Connect([list, input, generated, selectionText](UIElement*) {
        std::string text = input->GetText();
        if (text.empty()) text = "置顶项目 " + std::to_string(generated.Get());
        generated = generated.Get() + 1;
        list->InsertItem(0, text);
        selectionText = "已插入到第 1 项：" + text + "。";
    });
    auto remove = Make<Button>("删除选中");
    remove->OnClick().Connect([list, selectionText](UIElement*) {
        const int index = list->GetSelectedIndex();
        if (index < 0) {
            selectionText = "请先选择一个要删除的项目。";
            return;
        }
        const std::string text = list->GetSelectedItem();
        list->RemoveItem(index);
        selectionText = "已删除：" + text + "。";
    });
    auto reset = Make<Button>("重置数据");
    reset->OnClick().Connect([list, selectionText](UIElement*) {
        list->SetItems({ "收件箱", "今天", "本周", "已完成", "已归档", "垃圾箱" });
        list->ClearSelection();
        selectionText = "已重置为 6 个内存项目。";
    });

    auto clearItems = Make<Button>("清空项目");
    clearItems->OnClick().Connect([list, selectionText](UIElement*) {
        list->ClearItems();
        selectionText = "已清空全部内存项目。";
    });

    auto single = Make<Button>("单选");
    single->OnClick().Connect([list, selectionText](UIElement*) {
        list->SetSelectionMode(ListBoxSelectionMode::Single);
        list->ClearSelection();
        selectionText = "已切换为单选模式。";
    });
    auto multiple = Make<Button>("多选");
    multiple->OnClick().Connect([list, selectionText](UIElement*) {
        list->SetSelectionMode(ListBoxSelectionMode::Multiple);
        list->ClearSelection();
        selectionText = "已切换为多选模式；单击可逐项切换。";
    });
    auto extended = Make<Button>("扩展选择");
    extended->OnClick().Connect([list, selectionText](UIElement*) {
        list->SetSelectionMode(ListBoxSelectionMode::Extended);
        list->ClearSelection();
        selectionText = "已切换为扩展选择模式；支持 Ctrl / Shift。";
    });
    auto selectAll = Make<Button>("全选");
    selectAll->OnClick().Connect([list](UIElement*) { list->SelectAll(); });
    auto clear = Make<Button>("清除选择");
    clear->OnClick().Connect([list](UIElement*) { list->ClearSelection(); });

    auto custom = Make<ListBox>();
    custom->SetHeight(132.0f);
    custom->SetWidth(360.0f);
    auto important = Make<TextBlock>("★ 需要今天处理的自定义 UIElement");
    important->SetTextColor(Color::Hex("#E68A00"));
    auto synced = Make<TextBlock>("✓ 已同步到云端的自定义 UIElement");
    synced->SetTextColor(Color::Hex("#16803C"));
    custom->AddItem(important);
    custom->AddItem(synced);
    custom->AddItem("普通字符串项目仍可混用");

    static DemoListBoxDataSource virtualSource;
    auto virtualList = Make<ListBox>();
    virtualList->SetHeight(180.0f);
    virtualList->SetWidth(360.0f);
    virtualList->SetVirtualMode(10000, &virtualSource);
    virtualList->SetSelectionMode(ListBoxSelectionMode::Single);
    State<std::string> virtualStatusText{ "虚拟模式仅按需索引文本；可滚动、选择和键盘导航。" };
    auto virtualStatus = MakeStatus("");
    virtualStatus->Text->Bind(virtualStatusText, BindingMode::OneWay);
    virtualList->OnSelectionChanged().Connect([virtualStatusText](ListBox*, int index, const std::string& text) {
        virtualStatusText = "虚拟项目：第 " + std::to_string(index + 1) + " 项：" + text + "。";
    });
    auto jump = Make<Button>("定位到第 5000 项");
    jump->OnClick().Connect([virtualList](UIElement*) { virtualList->SetSelectedIndex(4999); });

    SamplePageSpec spec;
    spec.title = "ListBox(列表框)";
    spec.subtitle = "面向一维集合的选择控件：支持字符串/自定义项、动态编辑、多选、键盘操作、拖放与虚拟数据源。";
    spec.sections = {
        {
            "内存集合与选择",
            "单击选择；扩展选择模式下使用 Ctrl、Shift 或 Ctrl+A。双击会触发独立事件。",
            Column(10).Children({
                list,
                selectionStatus,
                Row(8).Children({ input, add, insert, remove }).Build(),
                Row(8).Children({ reset, clearItems, selectAll, clear }).Build(),
                Row(8).Children({ single, multiple, extended }).Build(),
            }).Build(),
        },
        {
            "自定义项目与拖放",
            "同一个 ListBox 可同时承载文本和自定义 UIElement；上方主列表已启用拖放。",
            custom,
        },
        {
            "虚拟模式",
            "适合数万到百万条记录。控件通过 ListBoxDataSource 按需取文本，不创建全部项目控件。",
            Column(8).Children({ virtualList, virtualStatus, jump }).Build(),
        },
    };
    spec.source =
        "auto list = Make<ListBox>();\n"
        "list->SetItems({ \"收件箱\", \"今天\", \"本周\" });\n"
        "list->SetSelectionMode(ListBoxSelectionMode::Extended);\n"
        "list->OnSelectionChanged().Connect(...);\n"
        "list->AddItem(\"新项目\");\n"
        "list->SetVirtualMode(10000, &dataSource);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
