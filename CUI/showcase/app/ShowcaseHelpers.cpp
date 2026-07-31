#include "ShowcaseHelpers.h"

using namespace CUI;
using namespace CUI::DSL;

std::shared_ptr<UIElement> CreateShowcaseText(
    const std::string& content,
    float size,
    const std::string& color,
    bool bold,
    const std::string& fontFamily) {
    auto text = Text(content).FontSize(size).Color(color).Build();
    if (bold) text->SetProperty("fontWeight", Value("Bold"));
    if (!fontFamily.empty()) text->SetProperty("fontFamily", Value(fontFamily));
    return text;
}

std::shared_ptr<UIElement> CreateShowcaseHeader(
    const std::string& title,
    const std::string& subtitle) {
    return Column(4).Children({
        CreateShowcaseText(title, 18.0f, "#FFFFFF", true),
        CreateShowcaseText(subtitle, 12.0f, "#888888")
    }).Build();
}

std::shared_ptr<UIElement> CreateDemoSurface(
    std::initializer_list<std::shared_ptr<UIElement>> children,
    float gap) {
    return Column(gap).Background("#252526").Padding(24).CornerRadius(6).Border("#333333", 1).Children(children).Build();
}

std::shared_ptr<UIElement> CreateRightPanel(
    std::initializer_list<std::shared_ptr<UIElement>> children) {
    return Column(10).Width(320).Background("#252526").Padding(16).Border("#333333", 1).Children(children).Build();
}

std::shared_ptr<UIElement> CreateRightScrollPanel(
    std::initializer_list<std::shared_ptr<UIElement>> children) {
    auto container = Column(8).Padding(16).Children(children).Build();
    auto scroll = std::make_shared<ScrollViewer>();
    scroll->SetProperty("width", Value(320.0f));
    scroll->SetProperty("background", Value("#252526"));
    scroll->SetProperty("borderBrush", Value("#333333"));
    scroll->SetProperty("borderThickness", Value(1.0f));
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
    return Row().Children({
        Column(16).FlexGrow(1.0f).Padding(20).Background("#1E1E1E").Children({
            CreateShowcaseHeader(title, subtitle),
            demo
        }).Build(),
        side
    }).Build();
}
