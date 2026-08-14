#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/ToggleButton.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildToggleButtonPage() {
    auto bold = std::make_shared<ToggleButton>("粗体");
	bold->SetFontWeight(FontWeight::Bold);
    auto italic = std::make_shared<ToggleButton>("斜体");
	italic->SetFontStyle(FontStyle::Italic);
    //下划线
    auto underline = std::make_shared<ToggleButton>("下划线");
	underline->SetIsUnderline(true);
    //删除线
    auto strikethrough = std::make_shared<ToggleButton>("删除线");
	strikethrough->SetIsStrikethrough(true);
    auto status = MakeStatus("当前状态：常规。");
    auto update = [bold, italic, underline, strikethrough, status]() {
        const bool isBold = bold->IsChecked();
        const bool isItalic = italic->IsChecked();

        status->SetFontWeight(isBold ? FontWeight::Bold : FontWeight::Normal);
        status->SetFontStyle(isItalic ? FontStyle::Italic : FontStyle::Normal);
        status->SetIsUnderline(false);
        status->SetIsStrikethrough(false);
        status->SetIsUnderline(underline->IsChecked());
        status->SetIsStrikethrough(strikethrough->IsChecked());
        std::string s;
        auto f = [&](bool b, const char* x) {
            if (b) s += s.empty() ? x : std::string(" + ") + x;
            };
        f(isBold, "粗体");f(isItalic, "斜体");
        f(underline->IsChecked(), "下划线");f(strikethrough->IsChecked(), "删除线");
        status->SetText("当前状态：" + (s.empty() ? "常规" : s) + "。");
    };
    bold->OnToggled().Connect([update](ToggleButton*, bool) { update(); });
    italic->OnToggled().Connect([update](ToggleButton*, bool) { update(); });
    underline->OnToggled().Connect([update](ToggleButton*, bool) { update(); });
    strikethrough->OnToggled().Connect([update](ToggleButton*, bool) { update(); });

    auto locked = std::make_shared<ToggleButton>("已锁定");
    locked->SetIsChecked(true);
    locked->SetIsEnabled(false);

    SamplePageSpec spec;
    spec.title = "ToggleButton(切换按钮)";
    spec.subtitle = "保持开或关的按钮。适用于粗体等模式，或工具栏按下状态。";
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
        "auto bold = std::make_shared<ToggleButton>(\"Bold\");\n"
        "bold->OnToggled().Connect([](ToggleButton*, bool on) {\n"
        "    // apply style\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
