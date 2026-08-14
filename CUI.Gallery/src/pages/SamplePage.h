#pragma once

#include "framework/controls/UIElement.h"
#include "framework/controls/TextBlock.h"
#include "framework/style/ThemeTokenId.h"
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace Gallery {

std::shared_ptr<CUI::UIElement> MakeLabel(
    const std::string& text,
    float size,
    CUI::ThemeTokenId token,
    bool bold = false);

std::shared_ptr<CUI::UIElement> MakeCard(
    std::initializer_list<std::shared_ptr<CUI::UIElement>> children,
    float gap = 16.0f);

std::shared_ptr<CUI::TextBlock> MakeStatus(const std::string& text);

struct SampleSection {
    std::string heading;
    std::string description;
    std::shared_ptr<CUI::UIElement> content;
};

struct SamplePageSpec {
    std::string title;
    std::string subtitle;
    std::vector<SampleSection> sections;
    std::string source;
};

std::shared_ptr<CUI::UIElement> BuildSamplePage(const SamplePageSpec& spec);

} // namespace Gallery
