#include "Gallery.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildRatingControlPage() {
    auto rating = RatingWidget();
    DSL::Borrow(rating).MaxRating(5);
    CUI::DSL::Borrow(rating).Step(0.5f);

    State<float> ratingValue{ 3.5f };
    rating->ValueProperty.Bind(ratingValue);

    auto statusValue = MakeComputed<std::string>([](float val) {
        return std::format("你的评分：{:.1f}", val);
    }, ratingValue);

    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    auto readOnly = RatingWidget();
    DSL::Borrow(readOnly).MaxRating(5);
    CUI::DSL::Borrow(readOnly).IsReadOnly(true);
    CUI::DSL::Borrow(readOnly).Value(4.0f);

    SamplePageSpec spec;
    spec.title = "RatingControl(评分)";
    spec.subtitle = "用星级表示评分。通过状态绑定同步所选分数。";
    spec.sections = {
        {
            "可交互",
            "半星步进。单击星星，或单击清除图标重置。",
            Column(10, { rating, status }),
        },
        {
            "只读",
            "SetIsReadOnly 显示不可更改的分数。",
            Column(8, {
                readOnly,
                MakeStatus("平均分：4.0"),
            }),
        },
    };
    spec.source =
        "State<float> ratingValue{ 3.5f };\n"
        "rating->ValueProperty.Bind(ratingValue, BindingMode::TwoWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery



