#pragma once

#include "framework/controls/UIElement.h"
#include "framework/controls/TextBlock.h"
#include "framework/style/ThemeTokenId.h"
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace Gallery {

CUI::Element MakeLabel(
    const std::string& text,
    float size,
    CUI::ThemeTokenId token,
    bool bold = false);

CUI::Element MakeCard(
    std::initializer_list<CUI::Element> children,
    float gap = 16.0f);

std::shared_ptr<CUI::TextBlock> MakeStatus(const std::string& text);

struct SampleSection {
    std::string heading;
    std::string description;
    CUI::Element content;
};

struct SamplePageSpec {
    std::string title;
    std::string subtitle;
    std::vector<SampleSection> sections;
    std::string source;
};

CUI::Element BuildSamplePage(const SamplePageSpec& spec);

} // namespace Gallery
