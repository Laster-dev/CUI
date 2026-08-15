#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/RatingControl.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildRatingControlPage() {
    auto rating = Make<RatingControl>();
    rating->SetMaxRating(5);
    rating->SetStep(0.5f);

    State<float> ratingValue{ 3.5f };
    rating->ValueProperty.Bind(ratingValue);

    auto statusValue = MakeComputed<std::string>([](float val) {
        return std::format("你的评分：{:.1f}", val);
    }, ratingValue);

    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto readOnly = Make<RatingControl>();
    readOnly->SetMaxRating(5);
    readOnly->SetIsReadOnly(true);
    readOnly->SetValue(4.0f);

    SamplePageSpec spec;
    spec.title = "RatingControl(评分)";
    spec.subtitle = "用星级表示评分。通过状态绑定同步所选分数。";
    spec.sections = {
        {
            "可交互",
            "半星步进。单击星星，或单击清除图标重置。",
            Column(10).Children({ rating, status }).Build(),
        },
        {
            "只读",
            "SetIsReadOnly 显示不可更改的分数。",
            Column(8).Children({
                readOnly,
                MakeStatus("平均分：4.0"),
            }).Build(),
        },
    };
    spec.source =
        "State<float> ratingValue{ 3.5f };\n"
        "rating->ValueProperty.Bind(ratingValue, BindingMode::TwoWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
