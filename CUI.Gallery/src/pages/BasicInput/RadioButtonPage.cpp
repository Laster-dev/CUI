#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/RadioButton.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildRadioButtonPage() {
    auto light = std::make_shared<RadioButton>("浅色");
    auto dark = std::make_shared<RadioButton>("深色");
    auto system = std::make_shared<RadioButton>("跟随系统");
    light->SetGroupName("theme");
    dark->SetGroupName("theme");
    system->SetGroupName("theme");
    light->SetState(CheckState::Checked);

    auto themeStatus = MakeStatus("主题：浅色。");
    auto onTheme = [themeStatus](CheckBox* sender, CheckState state) {
        if (state == CheckState::Checked) {
            themeStatus->SetText("主题：" + sender->GetText() + "。");
        }
    };
    light->OnCheckStateChanged().Connect(onTheme);
    dark->OnCheckStateChanged().Connect(onTheme);
    system->OnCheckStateChanged().Connect(onTheme);

    auto sizeS = std::make_shared<RadioButton>("小");
    auto sizeM = std::make_shared<RadioButton>("中");
    auto sizeL = std::make_shared<RadioButton>("大");
    sizeS->SetGroupName("size");
    sizeM->SetGroupName("size");
    sizeL->SetGroupName("size");
    sizeM->SetState(CheckState::Checked);

    auto sizeStatus = MakeStatus("大小：中。");
    auto onSize = [sizeStatus](CheckBox* sender, CheckState state) {
        if (state == CheckState::Checked) {
            sizeStatus->SetText("大小：" + sender->GetText() + "。");
        }
    };
    sizeS->OnCheckStateChanged().Connect(onSize);
    sizeM->OnCheckStateChanged().Connect(onSize);
    sizeL->OnCheckStateChanged().Connect(onSize);

    SamplePageSpec spec;
    spec.title = "RadioButton(单选按钮)";
    spec.subtitle = "单选按钮从一组中选择一项。SetGroupName 让各组互不影响。";
    spec.sections = {
        {
            "主题",
            "主题组中只能选一项。",
            Column(8).Children({
                light,
                dark,
                system,
                themeStatus,
            }).Build(),
        },
        {
            "大小",
            "同一页上的第二组不会干扰「主题」。",
            Column(8).Children({
                sizeS,
                sizeM,
                sizeL,
                sizeStatus,
            }).Build(),
        },
    };
    spec.source =
        "auto light = std::make_shared<RadioButton>(\"Light\");\n"
        "light->SetGroupName(\"theme\");\n"
        "light->SetState(CheckState::Checked);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
