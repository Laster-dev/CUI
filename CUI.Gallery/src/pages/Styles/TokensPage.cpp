#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildTokensPage() {
    ThemeManager& tm = ThemeManager::Instance();

    auto grid = WrapPanelWidget("Horizontal");
    grid.Gap(16.0f);

    for (const auto& name : tm.GetTokenNames()) {
        const D2D1_COLOR_F color = tm.GetFlatColor(name);

        auto chip = Container().Size(56.0f, 56.0f).CornerRadius(8.0f);
        chip->Background = color;
        chip->BorderToken = ThemeTokenId::CardBorder;
        chip->BorderThickness = 1.0f;

        auto item = Column(8, {
            chip,
            MakeLabel(name, 12.0f, ThemeTokenId::TextPrimary, false),
            MakeLabel(tm.GetColorHex(name), 11.0f, ThemeTokenId::TextMuted, false),
        });
        item->Width = 128.0f;
        grid->AddChild(item);
    }

    SamplePageSpec spec;
    spec.title = "Color Tokens(颜色令牌)";
    spec.subtitle = "CUI 全部主题色 Token 与当前主题下的实际色值。";
    spec.sections = {
        {
            "令牌矩阵",
            "色板取自当前生效主题；代码中通过 ThemeTokenId::Xxx 引用，切换主题后自动换色。",
            grid,
        },
    };
    spec.source =
        "control->SetBackgroundToken(ThemeTokenId::CardBackground);\n"
        "control->SetColorToken(ThemeTokenId::TextPrimary);\n"
        "D2D1_COLOR_F accent = ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor);\n"
        "std::string hex = ThemeManager::Instance().GetColorHex(\"accentColor\");\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
