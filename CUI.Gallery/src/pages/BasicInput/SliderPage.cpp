#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/Slider.h"
#include "framework/core/Value.h"
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildSliderPage() {
    auto volume = std::make_shared<Slider>();
    volume->SetMinimum(0.0f);
    volume->SetMaximum(100.0f);
    volume->SetStep(1.0f);
    volume->SetWidth(280.0f);
    auto volumeStatus = MakeStatus("");
    auto showVolume = [volumeStatus](Slider*, float value) {
        volumeStatus->SetText(std::format("音量：{:.0f}", value));
    };
    volume->OnValueChanged().Connect(showVolume);
    volume->SetValue(40.0f);
    showVolume(volume.get(), volume->GetValue());

    auto vertical = std::make_shared<Slider>();
    vertical->SetOrientation(Orientation::Vertical);
    vertical->SetMinimum(0.0f);
    vertical->SetMaximum(100.0f);
    vertical->SetWidth(32.0f);
    vertical->SetHeight(160.0f);
    auto verticalStatus = MakeStatus("");
    auto showVertical = [verticalStatus](Slider*, float value) {
        verticalStatus->SetText(std::format("电平：{:.0f}", value));
    };
    vertical->OnValueChanged().Connect(showVertical);
    vertical->SetValue(70.0f);
    showVertical(vertical.get(), vertical->GetValue());

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
        "auto slider = std::make_shared<Slider>();\n"
        "slider->SetMinimum(0.0f);\n"
        "slider->SetMaximum(100.0f);\n"
        "slider->OnValueChanged().Connect([](Slider*, float value) {\n"
        "    // use value\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
