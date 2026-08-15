#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/DatePicker.h"

#include <ctime>
#include <memory>
#include <string>

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
    auto picker = DatePickerWidget();
    State<std::string> selectedDate{ picker->GetFormattedDate() };
    picker->SelectedDate->Bind(selectedDate);

    auto statusValue = MakeComputed<std::string>([](const std::string& date) {
        return "当前选择日期：" + date + "。修改 State 或操作日历，两边会自动同步。";
    }, selectedDate);
    auto status = MakeStatus("");
    status->Text->Bind(statusValue, BindingMode::OneWay);

    auto today = Button("今天");
    today->OnClick().Connect([selectedDate](UIElement*) { selectedDate = TodayString(); });
    auto newYear = Button("元旦");
    newYear->OnClick().Connect([selectedDate](UIElement*) { selectedDate = "2027-01-01"; });
    auto spring = Button("春节示例");
    spring->OnClick().Connect([selectedDate](UIElement*) { selectedDate = "2027-02-06"; });
    auto birthday = Button("生日示例");
    birthday->OnClick().Connect([selectedDate](UIElement*) { selectedDate = "1990-06-15"; });

    auto disabled = DatePickerWidget();
    disabled->SetDate(2026, 12, 31);
    disabled->IsEnabledProperty = false;

    auto programmatic = Button("程序设置 2030-05-20");
    programmatic->OnClick().Connect([selectedDate](UIElement*) { selectedDate = "2030-05-20"; });

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
        "auto picker = DatePickerWidget();\n"
        "State<std::string> selectedDate{ picker->GetFormattedDate() };\n"
        "picker->SelectedDate->Bind(selectedDate);\n"
        "selectedDate = \"2030-05-20\"; // UI 自动刷新\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
