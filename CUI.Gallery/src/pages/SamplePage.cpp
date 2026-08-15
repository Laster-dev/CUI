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
    auto label = Text(text).FontSize(size).Build();
    label->ColorToken = token;
    if (bold) {
        label->FontWeight = FontWeight::Bold;
    }
    return label;
}

Element MakeCard(
    std::initializer_list<Element> children,
    float gap) {
    auto card = Column(gap).Padding(24).CornerRadius(6).Children(children).Build();
    card->BackgroundToken = ThemeTokenId::CardBackground;
    card->BorderToken = ThemeTokenId::CardBorder;
    card->BorderThickness = 1.0f;
    card->ClipToBounds = true;
    card->Margin = Thickness(0, 0, 0, 8);
    return card;
}

std::shared_ptr<TextBlock> MakeStatus(const std::string& text) {
    return std::static_pointer_cast<TextBlock>(
        MakeLabel(text, 12.0f, ThemeTokenId::TextSecondary, false));
}

namespace {

Element MakeSourceExpander(const std::string& source) {
    if (source.empty()) {
        return nullptr;
    }

    auto code = std::make_shared<TextBox>();
    code->Align = Alignment::Stretch;
    code->Height = 180.0f;
    code->FontFamily = "Consolas";
    code->FontSize = 12.0f;
    code->SetAcceptsReturn(true);
    code->SetTextWrapping(false);
    code->SetIsReadOnly(true);
    code->CornerRadius = 4.0f;
    code->BorderThickness = 1.0f;
    code->Padding = Thickness(10, 8, 10, 8);
    code->BackgroundToken = ThemeTokenId::InputBackground;
    code->BorderToken = ThemeTokenId::CardBorder;
    code->Text = source;

    auto expander = std::make_shared<Expander>("源代码");
    expander->SetIsExpanded(false);
    expander->SetContent(code);
    return expander;
}

Element MakeSectionCard(const SampleSection& section) {
    auto card = Column(8).Padding(24).CornerRadius(6);
    card.Add(MakeLabel(section.heading, 15.0f, ThemeTokenId::TextPrimary, true));
    if (!section.description.empty()) {
        card.Add(MakeLabel(section.description, 12.0f, ThemeTokenId::TextMuted, false));
    }
    if (section.content) {
        card.Add(section.content);
    }
    auto built = card.Build();
    built->BackgroundToken = ThemeTokenId::CardBackground;
    built->BorderToken = ThemeTokenId::CardBorder;
    built->BorderThickness = 1.0f;
    built->ClipToBounds = true;
    return built;
}

} // namespace

Element BuildSamplePage(const SamplePageSpec& spec) {
    auto main = Column(16).Padding(24);
    main.Add(Column(6, {
        MakeLabel(spec.title, 26.0f, ThemeTokenId::TextPrimary, true),
        MakeLabel(spec.subtitle, 14.0f, ThemeTokenId::TextMuted, false),
    }));

    for (const auto& section : spec.sections) {
        main.Add(MakeSectionCard(section));
    }
    if (auto source = MakeSourceExpander(spec.source)) {
        main.Add(source);
    }

    auto column = main.Build();
    column->BackgroundToken = ThemeTokenId::WindowBackground;
    column->AlignHorizontal = Alignment::Stretch;

    auto scroll = std::make_shared<ScrollViewer>();
    scroll->FlexGrow = 1.0f;
    scroll->Align = Alignment::Stretch;
    scroll->BackgroundToken = ThemeTokenId::WindowBackground;
    scroll->AddChild(column);
    return scroll;
}

} // namespace Gallery
