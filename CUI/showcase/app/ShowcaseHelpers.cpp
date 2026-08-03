#include "ShowcaseHelpers.h"
#include "framework/style/ThemeManager.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {
template <typename T>
std::shared_ptr<T> BindThemeToken(const std::shared_ptr<T>& element, const std::string& tokenProp, const std::string& tokenName) {
    if (element) {
        element->SetProperty(tokenProp, Value(tokenName));
        if (tokenProp == "theme.backgroundToken") {
            element->SetProperty("background", Value(ThemeManager::Instance().GetColor(tokenName)));
        } else if (tokenProp == "theme.borderToken") {
            element->SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor(tokenName)));
        } else if (tokenProp == "theme.colorToken") {
            element->SetProperty("color", Value(ThemeManager::Instance().GetColor(tokenName)));
        }
    }
    return element;
}

std::string ResolveLegacyColorToken(const std::string& color) {
    if (color == "#569CD6") return "accentColor";
    if (color == "#4EC9B0") return "accentColor";
    if (color == "#AAAAAA") return "textMuted";
    if (color == "#CCCCCC") return "textSecondary";
    if (color == "#888888") return "textMuted";
    if (color == "#B5CEA8") return "textSecondary";
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
        text->SetProperty("color", Value(color));
    }
    if (bold) text->SetProperty("fontWeight", Value("Bold"));
    if (!fontFamily.empty()) text->SetProperty("fontFamily", Value(fontFamily));
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
    auto surface = Column(gap).Padding(24).CornerRadius(6).Border(ThemeManager::Instance().GetColorHex("cardBorder"), 1).Children(children).Build();
    BindThemeToken(surface, "theme.backgroundToken", "cardBackground");
    BindThemeToken(surface, "theme.borderToken", "cardBorder");
    return surface;
}

std::shared_ptr<UIElement> CreateRightPanel(
    std::initializer_list<std::shared_ptr<UIElement>> children) {
    auto panel = Column(10).Width(320).Padding(16).Border(ThemeManager::Instance().GetColorHex("cardBorder"), 1).Children(children).Build();
    BindThemeToken(panel, "theme.backgroundToken", "cardBackground");
    BindThemeToken(panel, "theme.borderToken", "cardBorder");
    return panel;
}

std::shared_ptr<UIElement> CreateRightScrollPanel(
    std::initializer_list<std::shared_ptr<UIElement>> children) {
    auto container = Column(8).Padding(16).Children(children).Build();
    auto scroll = std::make_shared<ScrollViewer>();
    scroll->SetProperty("width", Value(320.0f));
    scroll->SetProperty("borderThickness", Value(1.0f));
    BindThemeToken(scroll, "theme.backgroundToken", "cardBackground");
    BindThemeToken(scroll, "theme.borderToken", "cardBorder");
    scroll->AddChild(container);
    return scroll;
}

std::shared_ptr<UIElement> CreatePropertyGrid(
    const ShowcaseContext& ctx,
    const std::shared_ptr<UIElement>& target) {
    auto grid = std::make_shared<PropertyGrid>();
    grid->SetProperty("width", Value(320.0f));
    grid->SetTargetElement(target, ctx.windowRef);
    return grid;
}

std::shared_ptr<UIElement> CreatePage(
    const std::string& title,
    const std::string& subtitle,
    const std::shared_ptr<UIElement>& demo,
    const std::shared_ptr<UIElement>& side) {
    auto mainColumn = Column(16).FlexGrow(1.0f).Padding(20).Children({
            CreateShowcaseHeader(title, subtitle),
            demo
        }).Build();
    BindThemeToken(mainColumn, "theme.backgroundToken", "windowBackground");
    return Row().Children({
        mainColumn,
        side
    }).Build();
}
