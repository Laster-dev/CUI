#pragma once
#include "MarkdownAst.h"
#include "../../core/Value.h"
#include <dwrite.h>
#include <cstdint>
#include <string>
#include <vector>

namespace CUI {

class GraphicsContext;

enum class MdSyntaxKind : uint8_t {
    None,
    Keyword,
    Type,
    String,
    Number,
    Comment,
    Preprocessor
};
struct MdRun {
    Rect bounds;
    std::string text;
    std::string font = "微软雅黑";
    float size = 14.0f;
    DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
    bool italic = false;
    bool code = false;
    bool link = false;
    bool strikethrough = false;
    MdSyntaxKind syntaxKind = MdSyntaxKind::None;
    std::string href;
    int plainStart = 0;
    int plainEnd = 0;
};

struct MdPaintBlock {
    enum class Type { Flow, Quote, Code, Hr, Table };
    Type type = Type::Flow;
    Rect bounds;
    int headingLevel = 0;
    std::vector<MdRun> runs;
    std::vector<std::string> codeLines;
    int tableCols = 0;
    int tableRows = 0;
    std::vector<float> tableRowHeights;
    std::string codeLanguage;
};

struct MdLayoutResult {
    float width = 0.0f;
    float height = 0.0f;
    std::string plain;
    std::vector<MdPaintBlock> blocks;
    std::vector<MdRun> runs;
};

MdLayoutResult LayoutMarkdown(
    GraphicsContext& ctx,
    const std::vector<MdBlock>& blocks,
    float width,
    bool showCodeLineNumbers);

} // namespace CUI
