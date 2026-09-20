#include "Gallery.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element MakeLabel(
    const std::string& text,
    float size,
    ThemeTokenId token,
    bool bold) {
    auto label = Text(text).AlignVertical(Alignment::Center).FontSize(size).Build();
    
    if (bold) {
                label->SetFontWeight(FontWeight::Bold);
    }
    return label;
}

Element MakeCard(
    std::initializer_list<Element> children,
    float gap) {
    auto card = Column(gap).Padding(24).CornerRadius(6).Children(children).Build();
        card->SetBackgroundToken(ThemeTokenId::CardBackground);
        card->SetBorderToken(ThemeTokenId::CardBorder);
        card->SetBorderThickness(1.0f);
        card->SetClipToBounds(true);
        card->SetMargin(Thickness(0, 0, 0, 8));
    return card;
}

std::shared_ptr<TextBlock> MakeStatus(const std::string& text) {
    auto label = std::static_pointer_cast<TextBlock>(
        MakeLabel(text, 12.0f, ThemeTokenId::TextSecondary, false));
        label->SetAlignVertical(Alignment::Center);
    return label;
}

namespace {

Element MakeSourceExpander(const std::string& source) {
    if (source.empty()) {
        return nullptr;
    }

    auto code = std::make_shared<TextBox>();
        code->SetAlign(Alignment::Stretch);
        code->SetHeight(180.0f);
    code->FontFamily = "Consolas";
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

Element MakeSectionCard(const SampleSection& section) {
    auto card = Column(12).Padding(24).CornerRadius(6);
    card.AddChild(MakeLabel(section.heading, 15.0f, ThemeTokenId::TextPrimary, true));
    if (!section.description.empty()) {
        card.AddChild(MakeLabel(section.description, 12.0f, ThemeTokenId::TextMuted, false));
    }
    if (section.content) {
        card.AddChild(section.content);
    }
    auto built = card.Build();
        built->SetBackgroundToken(ThemeTokenId::CardBackground);
        built->SetBorderToken(ThemeTokenId::CardBorder);
        built->SetBorderThickness(1.0f);
        built->SetClipToBounds(false);
    return built;
}

} // namespace

Element BuildSamplePage(const SamplePageSpec& spec) {
    auto main = Column(16).Padding(24);
    main.AddChild(Column(6, {
        MakeLabel(spec.title, 26.0f, ThemeTokenId::TextPrimary, true),
        MakeLabel(spec.subtitle, 14.0f, ThemeTokenId::TextMuted, false),
    }));

    for (const auto& section : spec.sections) {
        main.AddChild(MakeSectionCard(section));
    }
    if (auto source = MakeSourceExpander(spec.source)) {
        main.AddChild(source);
    }

    auto column = main.Build();
        column->SetBackgroundToken(ThemeTokenId::WindowBackground);
        column->SetAlignHorizontal(Alignment::Stretch);

    auto scroll = std::make_shared<ScrollViewer>();
        scroll->SetFlexGrow(1.0f);
        scroll->SetAlign(Alignment::Stretch);
        scroll->SetBackgroundToken(ThemeTokenId::WindowBackground);
    scroll->AddChild(column);
    return scroll;
}

} // namespace Gallery


