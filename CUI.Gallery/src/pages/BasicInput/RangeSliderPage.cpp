#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildRangeSliderPage() {
    auto price = Widgets::RangeSlider().Shared();
        price->SetMinimum(0.0f);
        price->SetMaximum(1000.0f);
        price->SetStep(10.0f);
        price->SetWidth(320.0f);

    State<float> lowerValue{ 200.0f };
    State<float> upperValue{ 800.0f };
    
    price->LowerValue.Bind(lowerValue);
    price->UpperValue.Bind(upperValue);

    auto statusValue = MakeComputed<std::string>([](float low, float up) {
        return std::format("价格：¥{:.0f} – ¥{:.0f}", low, up);
    }, lowerValue, upperValue);

    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "RangeSlider(范围滑块)";
    spec.subtitle = "两个滑块分别设置下限和上限。拖动范围的任一端。";
    spec.sections = {
        {
            "价格范围",
            "从 ¥0 到 ¥1,000 的筛选。两个滑块不能交叉。",
            Column(10, { price, status }),
        },
    };
    spec.source =
        "State<float> lowerValue{ 200.0f };\n"
        "State<float> upperValue{ 800.0f };\n"
        "range->LowerValue.Bind(lowerValue);\n"
        "range->UpperValue.Bind(upperValue);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery



