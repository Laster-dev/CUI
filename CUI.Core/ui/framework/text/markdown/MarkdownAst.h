#pragma once
#include <string>
#include <vector>

namespace CUI {

enum class MdInlineKind {
    Text,
    Strong,
    Emphasis,
    Code,
    Link,
    Break
};

struct MdInline {
    MdInlineKind kind = MdInlineKind::Text;
    std::string text;
    std::string href;
    std::vector<MdInline> children;
};

enum class MdBlockKind {
    Paragraph,
    Heading,
    List,
    ListItem,
    Quote,
    Code,
    ThematicBreak,
    Table
};

struct MdBlock {
    MdBlockKind kind = MdBlockKind::Paragraph;
    int headingLevel = 0;
    bool ordered = false;
    int listStart = 1;
    int listLevel = 0;
    std::string lang;
    std::string literal;
    std::vector<MdInline> inlines;
    std::vector<MdBlock> children;
    std::vector<std::vector<std::string>> tableRows;
    std::vector<int> tableAlign;
};

std::vector<MdBlock> ParseMarkdown(const std::string& source);
std::vector<MdInline> ParseMarkdownInlines(const std::string& source);

} // namespace CUI
