#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/ColorPicker.h"
#include "framework/controls/TextBlock.h"
#include "framework/style/ThemeTokenId.h"
#include <algorithm>
#include <format>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

int ToByte(float c) {
    return static_cast<int>(std::clamp(c, 0.0f, 1.0f) * 255.0f + 0.5f);
}

std::string ColorHex(Color c) {
    return std::format("#{:02X}{:02X}{:02X}", ToByte(c.r), ToByte(c.g), ToByte(c.b));
}

} // namespace

std::shared_ptr<UIElement> BuildColorPickerPage() {
    auto picker = Make<ColorPicker>();
    auto chip = Make<TextBlock>();
    chip->SetWidth(48.0f);
    chip->SetHeight(24.0f);
    chip->SetCornerRadius(4.0f);
    chip->SetBorderThickness(1.0f);
    chip->SetBorderToken(ThemeTokenId::CardBorder);

    State<Color> selectedColor{ Color(0, 0, 0, 1) };
    picker->SelectedColor.Bind(selectedColor);
    
    // Set default initial value from picker
    selectedColor = picker->GetSelectedColor();

    chip->Background.Bind(selectedColor, BindingMode::OneWay);

    auto hexValue = MakeComputed<std::string>([](Color color) {
        return ColorHex(color);
    }, selectedColor);

    auto hex = MakeStatus("");
    hex->Text.Bind(hexValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "ColorPicker(颜色选择器)";
    spec.subtitle = "从色板或色谱中选取颜色。预览色块使用实时 RGB 值。";
    spec.sections = {
        {
            "强调色",
            "打开选择器，色块和十六进制值会同步更新。",
            Column(10).Children({
                picker,
                Row(12).Children({ chip, hex }).Build(),
            }).Build(),
        },
    };
    spec.source =
        "State<Color> selectedColor{ Color(0, 0, 0, 1) };\n"
        "picker->SelectedColor.Bind(selectedColor);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
