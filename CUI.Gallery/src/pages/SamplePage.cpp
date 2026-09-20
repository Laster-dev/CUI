#include "Gallery.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element MakeLabel(
    const std::string& text,
    float size,
    ThemeTokenId token,
    bool bold) {
    CUI::Widgets::Ref label =Text(text).AlignVertical(Alignment::Center).FontSize(size).Build();
    
    if (bold) {
                label.FontWeight(FontWeight::Bold);
    }
    return label;
}

Element MakeCard(
    std::initializer_list<Element> children,
    float gap) {
    CUI::Widgets::Ref card =Column(gap).Padding(24).CornerRadius(6).Children(children).Build();
        card.BackgroundToken(ThemeTokenId::CardBackground);
        card.BorderToken(ThemeTokenId::CardBorder);
        card.BorderThickness(1.0f);
        card.ClipToBounds(true);
        card.Margin(Thickness(0, 0, 0, 8));
    return card;
}

std::shared_ptr<TextBlock> MakeStatus(const std::string& text) {
    CUI::Widgets::Ref label =std::static_pointer_cast<TextBlock>(
        MakeLabel(text, 12.0f, ThemeTokenId::TextSecondary, false));
        label.AlignVertical(Alignment::Center);
    return label;
}

namespace {

Element MakeSourceExpander(const std::string& source) {
    if (source.empty()) {
        return nullptr;
    }

    CUI::Widgets::Ref code = CUI::Widgets::TextBox().Shared();
        code.Align(Alignment::Stretch);
        code.Height(180.0f);
    code->FontFamily = "Consolas";
        code.FontSize(12.0f);
        code.AcceptsReturn(true);
        code.TextWrapping(false);
        code.IsReadOnly(true);
        code.CornerRadius(4.0f);
        code.BorderThickness(1.0f);
        code.Padding(Thickness(10, 8, 10, 8));
        code.BackgroundToken(ThemeTokenId::InputBackground);
        code.BorderToken(ThemeTokenId::CardBorder);
        code.Text(source);

    CUI::Widgets::Ref expander = CUI::Widgets::Expander("源代码").Shared();
        expander.IsExpanded(false);
        expander.Content(code);
    return expander;
}

Element MakeSectionCard(const SampleSection& section) {
    auto card =Column(12).Padding(24).CornerRadius(6);
    card.AddChild(MakeLabel(section.heading, 15.0f, ThemeTokenId::TextPrimary, true));
    if (!section.description.empty()) {
        card.AddChild(MakeLabel(section.description, 12.0f, ThemeTokenId::TextMuted, false));
    }
    if (section.content) {
        card.AddChild(section.content);
    }
    auto built =card.Build();
        built->ApplyBackgroundToken(ThemeTokenId::CardBackground);
        built->ApplyBorderToken(ThemeTokenId::CardBorder);
        built->ApplyBorderThickness(1.0f);
        built->ApplyClipToBounds(false);
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

    CUI::Widgets::Ref column =main.Build();
        column.BackgroundToken(ThemeTokenId::WindowBackground);
        column.AlignHorizontal(Alignment::Stretch);

    CUI::Widgets::Ref scroll = CUI::Widgets::ScrollViewer().Shared();
        scroll.FlexGrow(1.0f);
        scroll.Align(Alignment::Stretch);
        scroll.BackgroundToken(ThemeTokenId::WindowBackground);
    scroll->AddChild(column);
    return scroll;
}

} // namespace Gallery


