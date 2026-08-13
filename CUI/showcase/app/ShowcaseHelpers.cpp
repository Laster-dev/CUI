#include "ShowcaseHelpers.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"
#include "framework/controls/ScrollViewer.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {
template <typename T>
std::shared_ptr<T> BindThemeToken(const std::shared_ptr<T>& element, const std::string& tokenProp, const std::string& tokenName) {
    if (element) {
        ThemeTokenId id = ThemeTokenIdFromName(tokenName);
        if (tokenProp == "theme.backgroundToken") element->SetBackgroundToken(id);
        else if (tokenProp == "theme.hoverBackgroundToken") element->SetHoverBackgroundToken(id);
        else if (tokenProp == "theme.pressedBackgroundToken") element->SetPressedBackgroundToken(id);
        else if (tokenProp == "theme.borderToken") element->SetBorderToken(id);
        else if (tokenProp == "theme.focusedBorderToken") element->SetFocusedBorderToken(id);
        else if (tokenProp == "theme.colorToken") element->SetColorToken(id);
        else if (tokenProp == "theme.placeholderColorToken") element->SetPlaceholderColorToken(id);
        else if (tokenProp == "theme.dropdownBackgroundToken") element->SetDropdownBackgroundToken(id);
        else if (tokenProp == "theme.selectedItemBackgroundToken") element->SetSelectedItemBackgroundToken(id);
        else if (tokenProp == "theme.underlineColorToken") element->SetUnderlineColorToken(id);
        else if (tokenProp == "theme.activeUnderlineColorToken") element->SetActiveUnderlineColorToken(id);
        else if (tokenProp == "theme.caretColorToken") element->SetCaretColorToken(id);
    }
    return element;
}

std::string ResolveLegacyColorToken(const std::string& color) {
    if (color == "#569CD6" || color == "#4EC9B0" || color == "#007ACC" || color == "#0E639C") return "accentColor";
    if (color == "#10B981" || color == "#8E44AD" || color == "#D7A400" || color == "#C586C0" || color == "#9CDCFE") return "accentColor";
    if (color == "#6A9955" || color == "#CE9178" || color == "#DCDCAA") return "accentColor";
    if (color == "#D13438" || color == "#D16969") return "dangerColor";
    if (color == "#AAAAAA" || color == "#888888" || color == "#5A5A5A") return "textMuted";
    if (color == "#CCCCCC" || color == "#DBDBDB" || color == "#E5E5E5" || color == "#B5CEA8") return "textSecondary";
    if (color == "textPrimary" || color == "textSecondary" || color == "textMuted" ||
        color == "accentColor" || color == "accentForeground" || color == "dangerColor") {
        return color;
    }
    return "";
}
}

std::shared_ptr<UIElement> CreateShowcaseText(
    const std::string& content,
    float size,
    const std::string& color,
    bool bold,
    const std::string& fontFamily) {
    auto text = Text(content).FontSize(size).Build();
    const std::string mappedToken = ResolveLegacyColorToken(color);
    if (color.empty()) {
        BindThemeToken(text, "theme.colorToken", "textPrimary");
    } else if (!mappedToken.empty()) {
        BindThemeToken(text, "theme.colorToken", mappedToken);
    } else {
        BindThemeToken(text, "theme.colorToken", "textPrimary");
    }
    if (bold) text->SetFontWeight("Bold");
    if (!fontFamily.empty()) text->SetFontFamily(fontFamily);
    return text;
}

std::shared_ptr<UIElement> CreateShowcaseHeader(
    const std::string& title,
    const std::string& subtitle) {
    return Column(4).Children({
        BindThemeToken(std::static_pointer_cast<TextBlock>(CreateShowcaseText(title, 18.0f, "", true)), "theme.colorToken", "textPrimary"),
        BindThemeToken(std::static_pointer_cast<TextBlock>(CreateShowcaseText(subtitle, 12.0f, "", false)), "theme.colorToken", "textMuted")
    }).Build();
}

std::shared_ptr<UIElement> CreateDemoSurface(
    std::initializer_list<std::shared_ptr<UIElement>> children,
    float gap) {
    auto surface = Column(gap).Padding(24).CornerRadius(6).BorderToken(ThemeTokenId::CardBorder, 1).Children(children).Build();
    BindThemeToken(surface, "theme.backgroundToken", "cardBackground");
    BindThemeToken(surface, "theme.borderToken", "cardBorder");
    return surface;
}

std::shared_ptr<UIElement> CreatePage(
    const std::string& title,
    const std::string& subtitle,
    const std::shared_ptr<UIElement>& demo) {
    auto mainColumnBuilder = Column(16).FlexGrow(1.0f).Padding(20);
    mainColumnBuilder.Add(CreateShowcaseHeader(title, subtitle));
    if (demo) {
        mainColumnBuilder.Add(demo);
    }
    auto mainColumn = mainColumnBuilder.Build();
    BindThemeToken(mainColumn, "theme.backgroundToken", "windowBackground");

    auto mainScroll = std::make_shared<ScrollViewer>();
    mainScroll->SetFlexGrow(1.0f);
    mainScroll->SetMinWidth(240.0f);
    mainScroll->SetAlign(Alignment::Stretch);
    BindThemeToken(mainScroll, "theme.backgroundToken", "windowBackground");
    mainScroll->AddChild(mainColumn);
    return mainScroll;
}
