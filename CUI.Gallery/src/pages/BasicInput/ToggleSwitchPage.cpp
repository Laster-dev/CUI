#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/ToggleSwitch.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildToggleSwitchPage() {
    auto notify = Make<ToggleSwitch>();
    notify->SetHeader("通知");
    
    State<bool> notifyOn{ false };
    notify->IsOn.Bind(notifyOn);

    auto statusValue = MakeComputed<std::string>([](bool on) {
        return on ? "通知已开启。" : "通知已关闭。";
    }, notifyOn);

    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto wifi = Make<ToggleSwitch>();
    wifi->SetHeader("Wi-Fi");
    wifi->SetIsOn(true);

    auto locked = Make<ToggleSwitch>();
    locked->SetHeader("飞行模式");
    locked->IsEnabledProperty = false;

    SamplePageSpec spec;
    spec.title = "ToggleSwitch(开关)";
    spec.subtitle = "开关用于打开或关闭设置。通过状态绑定同步标签值。";
    spec.sections = {
        {
            "设置",
            "切换「通知」。Wi-Fi 默认开启。「飞行模式」不可用。",
            Column(12).Children({
                notify,
                wifi,
                locked,
                status,
            }).Build(),
        },
    };
    spec.source =
        "State<bool> notifyOn{ false };\n"
        "notify->IsOn.Bind(notifyOn);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
