#include "page/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/Expander.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/controls/TextBox.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> MakeLabel(
    const std::string& text,
    float size,
    ThemeTokenId token,
    bool bold) {
    auto label = Text(text).FontSize(size).Build();
    label->SetColorToken(token);
    if (bold) {
        label->SetFontWeight("Bold");
    }
    return label;
}

std::shared_ptr<UIElement> MakeCard(
    std::initializer_list<std::shared_ptr<UIElement>> children,
    float gap) {
    auto card = Column(gap).Padding(24).CornerRadius(6).Children(children).Build();
    card->SetBackgroundToken(ThemeTokenId::CardBackground);
    card->SetBorderToken(ThemeTokenId::CardBorder);
    card->SetBorderThickness(1.0f);
    return card;
}

std::shared_ptr<TextBlock> MakeStatus(const std::string& text) {
    return std::static_pointer_cast<TextBlock>(
        MakeLabel(text, 12.0f, ThemeTokenId::TextSecondary, false));
}

namespace {

std::shared_ptr<UIElement> MakeSourceExpander(const std::string& source) {
    if (source.empty()) {
        return nullptr;
    }

    auto code = std::make_shared<TextBox>();
    code->SetAlign(Alignment::Stretch);
    code->SetHeight(180.0f);
    code->SetFontFamily("Consolas");
    code->SetFontSize(12.0f);
    code->SetAcceptsReturn(true);
    code->SetTextWrapping(false);
    code->SetIsReadOnly(true);
    code->SetCornerRadius(4.0f);
    code->SetBorderThickness(1.0f);
    code->SetPadding(Thickness(10, 8, 10, 8));
    code->SetBackgroundToken(ThemeTokenId::InputBackground);
    code->SetBorderToken(ThemeTokenId::CardBorder);
    code->SetText(source);

    auto expander = std::make_shared<Expander>("源代码");
    expander->SetIsExpanded(false);
    expander->SetContent(code);
    return expander;
}

std::shared_ptr<UIElement> MakeSectionCard(const SampleSection& section) {
    auto card = Column(8).Padding(24).CornerRadius(6);
    card.Add(MakeLabel(section.heading, 15.0f, ThemeTokenId::TextPrimary, true));
    if (!section.description.empty()) {
        card.Add(MakeLabel(section.description, 12.0f, ThemeTokenId::TextMuted, false));
    }
    if (section.content) {
        card.Add(section.content);
    }
    auto built = card.Build();
    built->SetBackgroundToken(ThemeTokenId::CardBackground);
    built->SetBorderToken(ThemeTokenId::CardBorder);
    built->SetBorderThickness(1.0f);
    return built;
}

} // namespace

std::shared_ptr<UIElement> BuildSamplePage(const SamplePageSpec& spec) {
    auto main = Column(16).Padding(24);
    main.Add(Column(6).Children({
        MakeLabel(spec.title, 26.0f, ThemeTokenId::TextPrimary, true),
        MakeLabel(spec.subtitle, 14.0f, ThemeTokenId::TextMuted, false),
    }).Build());

    for (const auto& section : spec.sections) {
        main.Add(MakeSectionCard(section));
    }
    if (auto source = MakeSourceExpander(spec.source)) {
        main.Add(source);
    }

    auto column = main.Build();
    column->SetBackgroundToken(ThemeTokenId::WindowBackground);

    auto scroll = std::make_shared<ScrollViewer>();
    scroll->SetFlexGrow(1.0f);
    scroll->SetAlign(Alignment::Stretch);
    scroll->SetBackgroundToken(ThemeTokenId::WindowBackground);
    scroll->AddChild(column);
    return scroll;
}

} // namespace Gallery
