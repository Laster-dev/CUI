#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildToggleSwitchPage() {
    CUI::Widgets::Ref notify = Widgets::ToggleSwitch().Shared();
        notify.Header("通知");
    
    State<bool> notifyOn{ false };
    notify->IsOn.Bind(notifyOn);

    auto statusValue = MakeComputed<std::string>([](bool on) {
        return on ? "通知已开启。" : "通知已关闭。";
    }, notifyOn);

    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    CUI::Widgets::Ref wifi = Widgets::ToggleSwitch().Shared();
        wifi.Header("Wi-Fi");
        wifi.IsOn(true);

    CUI::Widgets::Ref locked = Widgets::ToggleSwitch().Shared();
        locked.Header("飞行模式");
        locked.IsEnabled(false);

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



