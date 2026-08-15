#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/TimePicker.h"

#include <memory>
#include <string>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildTimePickerPage() {
    auto picker = TimePickerWidget();
    State<std::string> selectedTime{ picker->GetFormattedTime() };
    picker->SelectedTime->Bind(selectedTime);

    auto statusValue = MakeComputed<std::string>([](const std::string& time) {
        return "当前选择时间：" + time + "。滚动小时/分钟列即可修改。";
    }, selectedTime);
    auto status = MakeStatus("");
    status->Text->Bind(statusValue, BindingMode::OneWay);

    auto morning = Button("上午 08:30")
        .OnClick([selectedTime](UIElement*) { selectedTime = "08:30"; });
    auto noon = Button("中午 12:00")
        .OnClick([selectedTime](UIElement*) { selectedTime = "12:00"; });
    auto evening = Button("晚上 18:30")
        .OnClick([selectedTime](UIElement*) { selectedTime = "18:30"; });
    auto midnight = Button("午夜 23:45")
        .OnClick([selectedTime](UIElement*) { selectedTime = "23:45"; });

    auto programmatic = Button("程序设置 06:15")
        .Width(200.0f)
        .OnClick([selectedTime](UIElement*) { selectedTime = "06:15"; });

    auto disabled = TimePickerWidget();
    disabled->SetTime(9, 0);
    disabled->IsEnabledProperty = false;

    auto second = TimePickerWidget();
    State<std::string> reminderTime{ "21:00" };
    second->SelectedTime->Bind(reminderTime);
    auto reminderStatusValue = MakeComputed<std::string>([](const std::string& time) {
        return "提醒时间：" + time;
    }, reminderTime);
    auto reminderStatus = MakeStatus("");
    reminderStatus->Text->Bind(reminderStatusValue, BindingMode::OneWay);
    auto setReminder = Button("设置提醒为 07:45")
        .OnClick([reminderTime](UIElement*) { reminderTime = "07:45"; });

    SamplePageSpec spec;
    spec.title = "TimePicker(时间选择器)";
    spec.subtitle = "支持小时/分钟滚轮选择；SelectedTime 提供 HH:MM 字符串的双向绑定。";
    spec.sections = {
        {
            "基础选择与双向绑定",
            "点击控件打开时间滚轮，分别滚动小时列和分钟列；程序设置 State 也会立即更新控件。",
            Column(10, {
                Row(12, {picker, status }),
                Row(8, {morning, noon, evening, midnight }),
                programmatic,
            }),
        },
        {
            "多个时间状态",
            "每个 TimePicker 可以绑定不同的 State，适合开始时间、结束时间、提醒时间等独立字段。",
            Row(12, { second, reminderStatus, setReminder }),
        },
        {
            "禁用状态",
            "禁用后保留显示值，但不能打开滚轮面板。",
            disabled,
        },
    };
    spec.source =
        "auto picker = TimePickerWidget();\n"
        "State<std::string> selectedTime{ \"14:30\" };\n"
        "picker->SelectedTime->Bind(selectedTime);\n"
        "selectedTime = \"06:15\"; // UI 自动刷新\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
