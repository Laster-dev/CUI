#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/Slider.h"
#include "framework/core/Value.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildSliderPage() {
    auto volume = Make<Slider>();
    volume->SetMinimum(0.0f);
    volume->SetMaximum(100.0f);
    volume->SetStep(1.0f);
    volume->SetWidth(280.0f);

    State<float> volumeValue{ 40.0f };
    volume->ValueProperty->Bind(volumeValue);

    auto volumeStatusValue = MakeComputed<std::string>([](float value) {
        return std::format("音量：{:.0f}", value);
    }, volumeValue);

    auto volumeStatus = MakeStatus("");
    volumeStatus->Text->Bind(volumeStatusValue, BindingMode::OneWay);

    auto vertical = Make<Slider>();
    vertical->SetOrientation(Orientation::Vertical);
    vertical->SetMinimum(0.0f);
    vertical->SetMaximum(100.0f);
    vertical->SetWidth(32.0f);
    vertical->SetHeight(160.0f);

    State<float> verticalValue{ 70.0f };
    vertical->ValueProperty->Bind(verticalValue);

    auto verticalStatusValue = MakeComputed<std::string>([](float value) {
        return std::format("电平：{:.0f}", value);
    }, verticalValue);

    auto verticalStatus = MakeStatus("");
    verticalStatus->Text->Bind(verticalStatusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "Slider(滑块)";
    spec.subtitle = "滑块从范围内选取一个值。拖动滑块，或使用方向键。";
    spec.sections = {
        {
            "水平",
            "0 到 100 的音量式滑块。",
            Column(10).Children({ volume, volumeStatus }).Build(),
        },
        {
            "垂直",
            "SetOrientation(Orientation::Vertical)，并给控件足够高度。",
            Column(10).Children({
                Row(16).Children({ vertical, verticalStatus }).Build(),
            }).Build(),
        },
    };
    spec.source =
        "State<float> volumeValue{ 40.0f };\n"
        "volume->Value->Bind(volumeValue);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
