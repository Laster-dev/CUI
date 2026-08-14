#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/ToggleSwitch.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildToggleSwitchPage() {
    auto notify = std::make_shared<ToggleSwitch>();
    notify->SetHeader("通知");
    auto status = MakeStatus("通知已关闭。");
    notify->OnToggled().Connect([status](ToggleSwitch*, bool on) {
        status->SetText(on ? "通知已开启。" : "通知已关闭。");
    });

    auto wifi = std::make_shared<ToggleSwitch>();
    wifi->SetHeader("Wi-Fi");
    wifi->SetIsOn(true);

    auto locked = std::make_shared<ToggleSwitch>();
    locked->SetHeader("飞行模式");
    locked->SetIsEnabled(false);

    SamplePageSpec spec;
    spec.title = "ToggleSwitch(开关)";
    spec.subtitle = "开关用于打开或关闭设置。用 SetHeader 设置轨道旁的标签。";
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
        "auto notify = std::make_shared<ToggleSwitch>();\n"
        "notify->SetHeader(\"Notifications\");\n"
        "notify->OnToggled().Connect([](ToggleSwitch*, bool on) {\n"
        "    // apply setting\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
