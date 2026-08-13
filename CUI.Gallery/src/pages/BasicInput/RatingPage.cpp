#include "pages/BasicInput/Pages.h"
#include "page/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/RatingControl.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildRatingPage() {
    auto rating = std::make_shared<RatingControl>();
    rating->SetMaxRating(5);
    rating->SetStep(0.5f);
    auto status = MakeStatus("");
    auto show = [status](RatingControl*, float value) {
        status->SetText(std::format("你的评分：{:.1f}", value));
    };
    rating->OnValueChanged().Connect(show);
    rating->SetValue(3.5f);
    show(rating.get(), rating->GetValue());

    auto readOnly = std::make_shared<RatingControl>();
    readOnly->SetMaxRating(5);
    readOnly->SetIsReadOnly(true);
    readOnly->SetValue(4.0f);

    SamplePageSpec spec;
    spec.title = "RatingControl(评分)";
    spec.subtitle = "用星级表示评分。单击或悬停设置分数，也可只读显示。";
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
        "auto rating = std::make_shared<RatingControl>();\n"
        "rating->SetMaxRating(5);\n"
        "rating->SetStep(0.5f);\n"
        "rating->OnValueChanged().Connect([](RatingControl*, float value) {\n"
        "    // use value\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
