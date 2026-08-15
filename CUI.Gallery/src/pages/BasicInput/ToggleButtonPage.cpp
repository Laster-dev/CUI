#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/ToggleButton.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildToggleButtonPage() {
    auto bold = Make<ToggleButton>("粗体");
    bold->SetFontWeight(FontWeight::Bold);
    
    auto italic = Make<ToggleButton>("斜体");
    italic->SetFontStyle(FontStyle::Italic);
    
    auto underline = Make<ToggleButton>("下划线");
    underline->SetIsUnderline(true);
    
    auto strikethrough = Make<ToggleButton>("删除线");
    strikethrough->SetIsStrikethrough(true);

    State<bool> boldChecked{ false };
    State<bool> italicChecked{ false };
    State<bool> underlineChecked{ false };
    State<bool> strikethroughChecked{ false };

    bold->IsOn.Bind(boldChecked);
    italic->IsOn.Bind(italicChecked);
    underline->IsOn.Bind(underlineChecked);
    strikethrough->IsOn.Bind(strikethroughChecked);

    auto statusText = MakeComputed<std::string>(
        [](bool b, bool i, bool u, bool s) {
            std::string text;
            if (b) text += text.empty() ? "粗体" : " + 粗体";
            if (i) text += text.empty() ? "斜体" : " + 斜体";
            if (u) text += text.empty() ? "下划线" : " + 下划线";
            if (s) text += text.empty() ? "删除线" : " + 删除线";
            return std::string("当前状态：") + (text.empty() ? "常规" : text) + "。";
        }, boldChecked, italicChecked, underlineChecked, strikethroughChecked);

    auto statusWeight = MakeComputed<CUI::FontWeight>(
        [](bool b) { return b ? FontWeight::Bold : FontWeight::Normal; }, boldChecked);
    auto statusStyle = MakeComputed<CUI::FontStyle>(
        [](bool i) { return i ? FontStyle::Italic : FontStyle::Normal; }, italicChecked);

    auto status = MakeStatus("");
    status->Text.Bind(statusText, BindingMode::OneWay);
    status->FontWeight.Bind(statusWeight, BindingMode::OneWay);
    status->FontStyle.Bind(statusStyle, BindingMode::OneWay);
    status->Underline.Bind(underlineChecked, BindingMode::OneWay);
    status->Strikethrough.Bind(strikethroughChecked, BindingMode::OneWay);

    auto locked = Make<ToggleButton>("已锁定");
    locked->SetIsChecked(true);
    locked->SetIsEnabled(false);

    SamplePageSpec spec;
    spec.title = "ToggleButton(切换按钮)";
    spec.subtitle = "保持开或关的按钮。通过状态绑定同步派生样式。";
    spec.sections = {
        {
            "文本样式",
            "单击以锁定样式。多个切换可同时打开。",
            Column(10).Children({
                Row(12).Children({ bold, italic, underline, strikethrough, locked }).Build(),
                status,
            }).Build(),
        },
    };
    spec.source =
        "State<bool> boldChecked{ false };\n"
        "bold->IsOn.Bind(boldChecked);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
