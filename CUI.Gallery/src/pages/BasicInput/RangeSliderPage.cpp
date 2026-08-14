#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/RangeSlider.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildRangeSliderPage() {
    auto price = std::make_shared<RangeSlider>();
    price->SetMinimum(0.0f);
    price->SetMaximum(1000.0f);
    price->SetStep(10.0f);
    price->SetWidth(320.0f);
    auto status = MakeStatus("");
    auto show = [status](RangeSlider*, float lower, float upper) {
        status->SetText(std::format("价格：¥{:.0f} – ¥{:.0f}", lower, upper));
    };
    price->OnValueChanged().Connect(show);
    price->SetRange(200.0f, 800.0f);
    show(price.get(), price->GetLowerValue(), price->GetUpperValue());

    SamplePageSpec spec;
    spec.title = "RangeSlider(范围滑块)";
    spec.subtitle = "两个滑块分别设置下限和上限。拖动范围的任一端。";
    spec.sections = {
        {
            "价格范围",
            "从 ¥0 到 ¥1,000 的筛选。两个滑块不能交叉。",
            Column(10).Children({ price, status }).Build(),
        },
    };
    spec.source =
        "auto range = std::make_shared<RangeSlider>();\n"
        "range->SetMinimum(0.0f);\n"
        "range->SetMaximum(1000.0f);\n"
        "range->OnValueChanged().Connect([](RangeSlider*, float lower, float upper) {\n"
        "    // use range\n"
        "});\n"
        "range->SetRange(200.0f, 800.0f);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
