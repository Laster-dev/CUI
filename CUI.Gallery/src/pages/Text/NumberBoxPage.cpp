#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/core/Value.h"
#include "framework/controls/NumberBox.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildNumberBoxPage() {
    auto basic = NumberBoxWidget(12.5).Width(150).Height(28).Build();
    basic->SetStep(0.5f);
    basic->SetToolTip("支持 ▲/▼、滚轮、上下键；可输入表达式如 1+2*3");

    State<float> basicValue{ 12.5f };
    basic->OnValueChanged().Connect([basicValue](NumberBox*, float value) {
        basicValue = value;
    });
    auto basicStatusValue = MakeComputed<std::string>([](float value) {
        return std::format("当前值：{:.1f}", value);
    }, basicValue);
    auto basicStatus = MakeStatus("");
    basicStatus->Text->Bind(basicStatusValue, BindingMode::OneWay);

    auto disabled = NumberBoxWidget(42).Width(150).Height(28).Build();
    disabled->SetIsEnabled(false);

    auto ranged = Make<NumberBox>();
    ranged->SetWidth(150.0f);
    ranged->SetHeight(28.0f);
    ranged->SetMinimum(0.0f);
    ranged->SetMaximum(100.0f);
    ranged->SetStep(5.0f);
    ranged->SetValue(60.0f);

    State<float> rangedValue{ 60.0f };
    ranged->OnValueChanged().Connect([rangedValue](NumberBox*, float value) {
        rangedValue = value;
    });
    auto rangedStatusValue = MakeComputed<std::string>([](float value) {
        return std::format("值被限制在 0–100 内：{:.0f}", value);
    }, rangedValue);
    auto rangedStatus = MakeStatus("");
    rangedStatus->Text->Bind(rangedStatusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "NumberBox(数字输入框)";
    spec.subtitle = "支持增减按钮、滚轮 / 方向键与数学表达式计算的数值输入框。";
    spec.sections = {
        {
            "常规用法",
            "▲/▼ 微调按钮按 Step 增减；聚焦后可直接输入，失焦或回车时提交。",
            Column(12).Children({
                basic,
                basicStatus,
                disabled,
            }).Build(),
        },
        {
            "范围与步长",
            "SetMinimum / SetMaximum 限制取值范围，SetStep 控制增减粒度。",
            Column(12).Children({
                ranged,
                rangedStatus,
            }).Build(),
        },
    };
    spec.source =
        "auto box = NumberBoxWidget(12.5).Width(150).Build();\n"
        "box->SetStep(0.5f);\n"
        "box->SetMinimum(0.0f);\n"
        "box->SetMaximum(100.0f);\n"
        "box->OnValueChanged().Connect([](NumberBox*, float value) { ... });\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
