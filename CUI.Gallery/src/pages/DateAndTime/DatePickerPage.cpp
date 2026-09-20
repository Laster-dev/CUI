#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include <ctime>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {
namespace {

std::string TodayString() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &local);
    return buffer;
}

} // namespace

Element BuildDatePickerPage() {
    CUI::Widgets::Ref picker = Widgets::DatePicker().Shared();
    State<std::string> selectedDate{ picker->GetFormattedDate() };
    picker->SelectedDate->Bind(selectedDate);

    auto statusValue = MakeComputed<std::string>([](const std::string& date) {
        return "当前选择日期：" + date + "。修改 State 或操作日历，两边会自动同步。";
    }, selectedDate);
    auto status = MakeStatus("");
    status->Text->Bind(statusValue, BindingMode::OneWay);

    auto today = Button("今天")
        .OnClick([selectedDate](UIElement*) { selectedDate = TodayString(); });
    auto newYear = Button("元旦")
        .OnClick([selectedDate](UIElement*) { selectedDate = "2027-01-01"; });
    auto spring = Button("春节示例")
        .OnClick([selectedDate](UIElement*) { selectedDate = "2027-02-06"; });
    auto birthday = Button("生日示例")
        .OnClick([selectedDate](UIElement*) { selectedDate = "1990-06-15"; });

    CUI::Widgets::Ref disabled = Widgets::DatePicker().Shared();
        disabled.Date(2026, 12, 31);
        disabled.IsEnabled(false);

    auto programmatic = Button("程序设置 2030-05-20")
        .OnClick([selectedDate](UIElement*) { selectedDate = "2030-05-20"; });

    SamplePageSpec spec;
    spec.title = "DatePicker(日期选择器)";
    spec.subtitle = "支持日、月、年三级日历视图；SelectedDate 提供 YYYY-MM-DD 字符串的双向绑定。";
    spec.sections = {
        {
            "基础选择与双向绑定",
            "点击日期按钮打开日历；点击标题可进入月份/年份视图，再选择目标日期。",
            Column(10, {
                Row(12, {picker, status }),
                Row(8, {today, newYear, spring, birthday }),
                programmatic,
            }),
        },
        {
            "禁用状态",
            "禁用后仍显示当前值，但不能打开日历或修改日期。",
            disabled,
        },
    };
    spec.source =
        "auto picker = Widgets::DatePicker().Shared();\n"
        "State<std::string> selectedDate{ picker->GetFormattedDate() };\n"
        "picker->SelectedDate->Bind(selectedDate);\n"
        "selectedDate = \"2030-05-20\"; // UI 自动刷新\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery



