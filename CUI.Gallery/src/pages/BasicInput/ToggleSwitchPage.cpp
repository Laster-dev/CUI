#include "Gallery.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildToggleSwitchPage() {
    auto notify = ToggleSwitchWidget();
    DSL::Borrow(notify).Header("通知");
    
    State<bool> notifyOn{ false };
    notify->IsOn.Bind(notifyOn);

    auto statusValue = MakeComputed<std::string>([](bool on) {
        return on ? "通知已开启。" : "通知已关闭。";
    }, notifyOn);

    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto wifi = ToggleSwitchWidget();
    DSL::Borrow(wifi).Header("Wi-Fi");
    DSL::Borrow(wifi).IsOn(true);

    auto locked = ToggleSwitchWidget();
    DSL::Borrow(locked).Header("飞行模式");
    CUI::DSL::Borrow(locked).IsEnabled(false);

    SamplePageSpec spec;
    spec.title = "ToggleSwitch(开关)";
    spec.subtitle = "开关用于打开或关闭设置。通过状态绑定同步标签值。";
    spec.sections = {
        {
            "设置",
            "切换「通知」。Wi-Fi 默认开启。「飞行模式」不可用。",
            Column(12, {
                notify,
                wifi,
                locked,
                status,
            }),
        },
    };
    spec.source =
        "State<bool> notifyOn{ false };\n"
        "notify->IsOn.Bind(notifyOn);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery



