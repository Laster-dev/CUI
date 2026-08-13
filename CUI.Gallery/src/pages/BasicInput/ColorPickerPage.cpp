#include "pages/BasicInput/Pages.h"
#include "page/SamplePage.h"

#include "framework/core/CUIDsl.h"
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

std::string ColorHex(D2D1_COLOR_F c) {
    return std::format("#{:02X}{:02X}{:02X}", ToByte(c.r), ToByte(c.g), ToByte(c.b));
}

} // namespace

std::shared_ptr<UIElement> BuildColorPickerPage() {
    auto picker = std::make_shared<ColorPicker>();
    auto chip = std::make_shared<TextBlock>();
    chip->SetWidth(48.0f);
    chip->SetHeight(24.0f);
    chip->SetCornerRadius(4.0f);
    chip->SetBorderThickness(1.0f);
    chip->SetBorderToken(ThemeTokenId::CardBorder);
    auto hex = MakeStatus("");

    auto apply = [chip, hex](D2D1_COLOR_F color) {
        chip->SetBackground(color);
        hex->SetText(ColorHex(color));
    };
    picker->OnColorChanged().Connect([apply](ColorPicker*, D2D1_COLOR_F color) {
        apply(color);
    });
    apply(picker->GetSelectedColor());

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
        "auto picker = std::make_shared<ColorPicker>();\n"
        "picker->OnColorChanged().Connect([](ColorPicker*, D2D1_COLOR_F color) {\n"
        "    preview->SetBackground(color); // no background token\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
