#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/Expander.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/controls/TextBox.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element MakeLabel(
    const std::string& text,
    float size,
    ThemeTokenId token,
    bool bold) {
    auto label = Text(text).AlignVertical(Alignment::Center).FontSize(size).Build();
    CUI::DSL::Borrow(label).ForegroundToken(token);
    if (bold) {
        CUI::DSL::Borrow(label).FontWeight(FontWeight::Bold);
    }
    return label;
}

Element MakeCard(
    std::initializer_list<Element> children,
    float gap) {
    auto card = Column(gap).Padding(24).CornerRadius(6).Children(children).Build();
    CUI::DSL::Borrow(card).BackgroundToken(ThemeTokenId::CardBackground);
    CUI::DSL::Borrow(card).BorderToken(ThemeTokenId::CardBorder);
    CUI::DSL::Borrow(card).BorderThickness(1.0f);
    CUI::DSL::Borrow(card).ClipToBounds(true);
    CUI::DSL::Borrow(card).Margin(Thickness(0, 0, 0, 8));
    return card;
}

std::shared_ptr<TextBlock> MakeStatus(const std::string& text) {
    auto label = std::static_pointer_cast<TextBlock>(
        MakeLabel(text, 12.0f, ThemeTokenId::TextSecondary, false));
    DSL::Borrow(label).AlignVertical(Alignment::Center);
    return label;
}

namespace {

Element MakeSourceExpander(const std::string& source) {
    if (source.empty()) {
        return nullptr;
    }

    auto code = std::make_shared<TextBox>();
    CUI::DSL::Borrow(code).Align(Alignment::Stretch);
    CUI::DSL::Borrow(code).Height(180.0f);
    code->FontFamily = "Consolas";
    CUI::DSL::Borrow(code).FontSize(12.0f);
    CUI::DSL::Borrow(code).AcceptsReturn(true);
    CUI::DSL::Borrow(code).TextWrapping(false);
    CUI::DSL::Borrow(code).IsReadOnly(true);
    CUI::DSL::Borrow(code).CornerRadius(4.0f);
    CUI::DSL::Borrow(code).BorderThickness(1.0f);
    CUI::DSL::Borrow(code).Padding(Thickness(10, 8, 10, 8));
    CUI::DSL::Borrow(code).BackgroundToken(ThemeTokenId::InputBackground);
    CUI::DSL::Borrow(code).BorderToken(ThemeTokenId::CardBorder);
    CUI::DSL::Borrow(code).Text(source);

    auto expander = std::make_shared<Expander>("源代码");
    CUI::DSL::Borrow(expander).IsExpanded(false);
    CUI::DSL::Borrow(expander).Content(code);
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
    CUI::DSL::Borrow(built).BackgroundToken(ThemeTokenId::CardBackground);
    CUI::DSL::Borrow(built).BorderToken(ThemeTokenId::CardBorder);
    CUI::DSL::Borrow(built).BorderThickness(1.0f);
    CUI::DSL::Borrow(built).ClipToBounds(false);
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
    CUI::DSL::Borrow(column).BackgroundToken(ThemeTokenId::WindowBackground);
    CUI::DSL::Borrow(column).AlignHorizontal(Alignment::Stretch);

    auto scroll = std::make_shared<ScrollViewer>();
    CUI::DSL::Borrow(scroll).FlexGrow(1.0f);
    CUI::DSL::Borrow(scroll).Align(Alignment::Stretch);
    CUI::DSL::Borrow(scroll).BackgroundToken(ThemeTokenId::WindowBackground);
    scroll->AddChild(column);
    return scroll;
}

} // namespace Gallery


