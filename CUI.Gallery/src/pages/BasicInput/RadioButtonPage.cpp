#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/RadioButton.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildRadioButtonPage() {
    auto light = RadioButtonTile("浅色");
    auto dark = RadioButtonTile("深色");
    auto system = RadioButtonTile("跟随系统");
    light->SetGroupName("theme");
    dark->SetGroupName("theme");
    system->SetGroupName("theme");

    State<bool> lightChecked{ true };
    State<bool> darkChecked{ false };
    State<bool> systemChecked{ false };

    light->Checked.Bind(lightChecked);
    dark->Checked.Bind(darkChecked);
    system->Checked.Bind(systemChecked);

    auto themeStatusValue = MakeComputed<std::string>(
        [](bool l, bool d, bool s) {
            if (l) return "主题：浅色。";
            if (d) return "主题：深色。";
            if (s) return "主题：跟随系统。";
            return "请选择主题。";
        }, lightChecked, darkChecked, systemChecked);

    auto themeStatus = MakeStatus("");
    themeStatus->Text.Bind(themeStatusValue, BindingMode::OneWay);

    auto sizeS = RadioButtonTile("小");
    auto sizeM = RadioButtonTile("中");
    auto sizeL = RadioButtonTile("大");
    sizeS->SetGroupName("size");
    sizeM->SetGroupName("size");
    sizeL->SetGroupName("size");

    State<bool> sizeSChecked{ false };
    State<bool> sizeMChecked{ true };
    State<bool> sizeLChecked{ false };

    sizeS->Checked.Bind(sizeSChecked);
    sizeM->Checked.Bind(sizeMChecked);
    sizeL->Checked.Bind(sizeLChecked);

    auto sizeStatusValue = MakeComputed<std::string>(
        [](bool s, bool m, bool l) {
            if (s) return "大小：小。";
            if (m) return "大小：中。";
            if (l) return "大小：大。";
            return "请选择大小。";
        }, sizeSChecked, sizeMChecked, sizeLChecked);

    auto sizeStatus = MakeStatus("");
    sizeStatus->Text.Bind(sizeStatusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "RadioButton(单选按钮)";
    spec.subtitle = "单选按钮从一组中选择一项。SetGroupName 让各组互不影响。";
    spec.sections = {
        {
            "主题",
            "主题组中只能选一项。",
            Column(8, {
                light,
                dark,
                system,
                themeStatus,
            }),
        },
        {
            "大小",
            "同一页上的第二组不会干扰「主题」。",
            Column(8, {
                sizeS,
                sizeM,
                sizeL,
                sizeStatus,
            }),
        },
    };
    spec.source =
        "State<bool> lightChecked{ true };\n"
        "light->Checked.Bind(lightChecked);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
