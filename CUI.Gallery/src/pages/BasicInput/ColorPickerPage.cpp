#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
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

Element BuildColorPickerPage() {
    CUI::Widgets::Ref picker = Widgets::ColorPicker().Shared();
    auto chip = Text()
        .Width(48.0f)
        .Height(24.0f)
        .CornerRadius(4.0f)
        .BorderThickness(1.0f);
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
            Column(10, {
                picker,
                Row(12, {chip, hex }),
            }),
        },
    };
    spec.source =
        "State<Color> selectedColor{ Color(0, 0, 0, 1) };\n"
        "picker->SelectedColor.Bind(selectedColor);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery



