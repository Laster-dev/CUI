#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildNumberBoxPage() {
    auto basic = Widgets::NumberBox().Width(150).Height(28).Shared();
    basic->SetValue(12.5);
        basic->SetStep(0.5f);
    basic->ToolTip = "支持 ▲/▼、滚轮、上下键；可输入表达式如 1+2*3";

    State<float> basicValue{ 12.5f };
    basic->ValueProperty.Bind(basicValue, BindingMode::TwoWay);
    auto basicStatusValue = MakeComputed<std::string>([](float value) {
        return std::format("当前值：{:.1f}", value);
    }, basicValue);
    auto basicStatus = MakeStatus("");
    basicStatus->Text.Bind(basicStatusValue, BindingMode::OneWay);

    auto disabled = Widgets::NumberBox().Width(150).Height(28).Shared();
    disabled->SetValue(42);
        disabled->SetIsEnabled(false);

    auto ranged = Widgets::NumberBox().Shared();
        ranged->SetWidth(150.0f);
        ranged->SetHeight(28.0f);
        ranged->SetMinimum(0.0f);
        ranged->SetMaximum(100.0f);
        ranged->SetStep(5.0f);
        ranged->SetValue(60.0f);

    State<float> rangedValue{ 60.0f };
    ranged->ValueProperty.Bind(rangedValue, BindingMode::TwoWay);
    auto rangedStatusValue = MakeComputed<std::string>([](float value) {
        return std::format("值被限制在 0–100 内：{:.0f}", value);
    }, rangedValue);
    auto rangedStatus = MakeStatus("");
    rangedStatus->Text.Bind(rangedStatusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "NumberBox(数字输入框)";
    spec.subtitle = "支持增减按钮、滚轮 / 方向键与数学表达式计算的数值输入框。";
    spec.sections = {
        {
            "常规用法",
            "▲/▼ 微调按钮按 Step 增减；聚焦后可直接输入，失焦或回车时提交。",
            Column(12, {
                basic,
                basicStatus,
                disabled,
            }),
        },
        {
            "范围与步长",
            "SetMinimum / SetMaximum 限制取值范围，SetStep 控制增减粒度。",
            Column(12, {
                ranged,
                rangedStatus,
            }),
        },
    };
    spec.source =
        "auto box = Widgets::NumberBox(12.5).Width(150).Shared();\n"
        "box.Step(0.5f);\n"
        "box.Minimum(0.0f);\n"
        "box.Maximum(100.0f);\n"
        "State<float> value{ 12.5f };\n"
        "box->ValueProperty.Bind(value, BindingMode::TwoWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery

