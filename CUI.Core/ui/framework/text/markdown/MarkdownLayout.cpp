#include "MarkdownLayout.h"
#include "../../render/GraphicsContext.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>

namespace CUI {
namespace {

struct Style {
    std::string font = "微软雅黑";
    float size = 14.0f;
    DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
    bool italic = false;
    bool code = false;
    bool link = false;
    std::string href;
    bool strikethrough = false;
    MdSyntaxKind syntaxKind = MdSyntaxKind::None;
};

size_t NextUtf8(const std::string& s, size_t i) {
    if (i >= s.size()) {
        return s.size();
    }
    const unsigned c = static_cast<unsigned char>(s[i]);
    if (c < 0x80) {
        return i + 1;
    }
    if ((c & 0xE0) == 0xC0) {
        return (std::min)(i + 2, s.size());
    }
    if ((c & 0xF0) == 0xE0) {
        return (std::min)(i + 3, s.size());
    }
    if ((c & 0xF8) == 0xF0) {
        return (std::min)(i + 4, s.size());
    }
    return i + 1;
}

size_t NextAtom(const std::string& s, size_t i) {
    if (i >= s.size()) {
        return s.size();
    }
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (c == ' ' || c == '\t') {
        size_t j = i;
        while (j < s.size() && (s[j] == ' ' || s[j] == '\t')) {
            ++j;
        }
        return j;
    }
    if (c < 0x80 && (std::isalnum(c) || c == '_' || c == '.' || c == '/' || c == '-' || c == '@')) {
        size_t j = i;
        while (j < s.size()) {
            const unsigned char d = static_cast<unsigned char>(s[j]);
            if (d < 0x80 && (std::isalnum(d) || d == '_' || d == '.' || d == '/' || d == '-' || d == '@')) {
                ++j;
            } else {
                break;
            }
        }
        return j;
    }
    return NextUtf8(s, i);
}

Size MeasureStyled(
    GraphicsContext& ctx,
    const std::string& text,
    const Style& style) {
    if (text.empty()) {
        return Size(0.0f, style.size + 4.0f);
    }
    if (!style.italic) {
        return ctx.MeasureText(text, style.font, style.size, style.weight);
    }
    GraphicsContext::TextLayoutOptions opt;
    opt.wrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
    auto layout = ctx.CreateTextLayout(Utf8ToUtf16(text), style.font, style.size, opt, style.weight);
    if (!layout) {
        return ctx.MeasureText(text, style.font, style.size, style.weight);
    }
    const std::wstring w = Utf8ToUtf16(text);
    DWRITE_TEXT_RANGE range{ 0, static_cast<UINT32>(w.size()) };
    layout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, range);
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    return Size(
        (std::max)(metrics.widthIncludingTrailingWhitespace, metrics.width) + 1.0f,
        (std::max)(metrics.height, style.size + 2.0f));
}

struct Flow {
    GraphicsContext& ctx;
    MdLayoutResult& out;
    float x0 = 0.0f;
    float maxX = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    float lineH = 22.0f;
    float lineStartY = 0.0f;
    std::vector<MdRun> lineRuns;

    void NewLine() {
        x = x0;
        y += lineH;
        lineStartY = y;
        lineRuns.clear();
    }

    void Emit(const std::string& text, const Style& style) {
        if (text.empty()) {
            return;
        }
        const Size sz = MeasureStyled(ctx, text, style);
        MdRun run;
        run.bounds = Rect(x, y, sz.width, (std::max)(sz.height, lineH * 0.85f));
        run.text = text;
        run.font = style.font;
        run.size = style.size;
        run.weight = style.weight;
        run.italic = style.italic;
        run.code = style.code;
        run.link = style.link;
        run.href = style.href;
        run.strikethrough = style.strikethrough;
        run.syntaxKind = style.syntaxKind;
        run.plainStart = static_cast<int>(out.plain.size());
        out.plain += text;
        run.plainEnd = static_cast<int>(out.plain.size());
        out.runs.push_back(run);
        lineRuns.push_back(run);
        x += sz.width;
    }

    void AppendText(const std::string& text, const Style& style) {
        size_t i = 0;
        while (i < text.size()) {
            if (text[i] == '\n') {
                out.plain.push_back('\n');
                NewLine();
                ++i;
                continue;
            }
            const size_t j = NextAtom(text, i);
            std::string atom = text.substr(i, j - i);
            Size sz = MeasureStyled(ctx, atom, style);
            if (x > x0 + 0.5f && x + sz.width > maxX) {
                NewLine();
            }
            if (sz.width > maxX - x0 && atom.size() > 1) {
                size_t k = 0;
                while (k < atom.size()) {
                    const size_t n = NextUtf8(atom, k);
                    std::string ch = atom.substr(k, n - k);
                    Size cs = MeasureStyled(ctx, ch, style);
                    if (x > x0 + 0.5f && x + cs.width > maxX) {
                        NewLine();
                    }
                    Emit(ch, style);
                    k = n;
                }
            } else {
                Emit(atom, style);
            }
            i = j;
        }
    }

    void AppendInlines(const std::vector<MdInline>& inlines, Style style) {
        for (const auto& node : inlines) {
            switch (node.kind) {
            case MdInlineKind::Text:
                AppendText(node.text, style);
                break;
            case MdInlineKind::Code: {
                Style code = style;
                code.font = "Consolas";
                code.size = (std::max)(11.0f, style.size - 1.0f);
                code.weight = DWRITE_FONT_WEIGHT_NORMAL;
                code.italic = false;
                code.code = true;
                AppendText(node.text, code);
                break;
            }
            case MdInlineKind::Strong: {
                Style next = style;
                next.weight = DWRITE_FONT_WEIGHT_BOLD;
                AppendInlines(node.children, next);
                break;
            }
            case MdInlineKind::Emphasis: {
                Style next = style;
                next.italic = true;
                AppendInlines(node.children, next);
                break;
            }
            case MdInlineKind::Strikethrough: {
                Style next = style;
                next.strikethrough = true;
                AppendInlines(node.children, next);
                break;
            }            case MdInlineKind::Link: {
                Style next = style;
                next.link = true;
                next.href = node.href;
                AppendInlines(node.children, next);
                break;
            }
            case MdInlineKind::Break:
                out.plain.push_back('\n');
                NewLine();
                break;
            }
        }
    }
};

std::string ToLowerAscii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool IsIdentifierStart(unsigned char ch) {
    return std::isalpha(ch) || ch == '_';
}

bool IsIdentifierPart(unsigned char ch) {
    return std::isalnum(ch) || ch == '_';
}

bool IsKeyword(const std::string& language, const std::string& token) {
    static const char* cpp[] = {
        "auto", "bool", "break", "case", "catch", "class", "const", "constexpr", "continue", "default",
        "delete", "do", "else", "enum", "explicit", "false", "float", "for", "if", "inline", "int",
        "namespace", "new", "nullptr", "private", "protected", "public", "return", "static", "struct",
        "switch", "template", "this", "true", "try", "typename", "using", "virtual", "void", "while"
    };
    static const char* python[] = {
        "and", "as", "assert", "async", "await", "break", "class", "continue", "def", "del", "elif",
        "else", "except", "false", "finally", "for", "from", "if", "import", "in", "is", "lambda", "none",
        "not", "or", "pass", "raise", "return", "true", "try", "while", "with", "yield"
    };
    static const char* json[] = { "true", "false", "null" };
    const char* const* words = cpp;
    size_t count = sizeof(cpp) / sizeof(cpp[0]);
    if (language == "py" || language == "python") {
        words = python;
        count = sizeof(python) / sizeof(python[0]);
    } else if (language == "json") {
        words = json;
        count = sizeof(json) / sizeof(json[0]);
    }
    for (size_t index = 0; index < count; ++index) {
        if (token == words[index]) {
            return true;
        }
    }
    return false;
}

bool IsTypeToken(const std::string& token) {
    static const char* types[] = {
        "bool", "char", "double", "float", "int", "long", "short", "size_t", "std", "string", "uint32_t",
        "uint64_t", "vector", "wstring"
    };
    for (const char* type : types) {
        if (token == type) {
            return true;
        }
    }
    return false;
}

void AppendHighlightedCode(Flow& flow, const std::string& line, const std::string& language, Style style) {
    const std::string lang = ToLowerAscii(language);
    const bool python = lang == "py" || lang == "python";
    const bool json = lang == "json";
    const bool markup = lang == "xml" || lang == "html" || lang == "xaml" || lang == "svg";
    size_t index = 0;
    while (index < line.size()) {
        const unsigned char current = static_cast<unsigned char>(line[index]);
        if ((line.compare(index, 2, "//") == 0 && !python && !json)
            || (python && line[index] == '#')
            || (markup && line.compare(index, 4, "<!--") == 0)) {
            Style comment = style;
            comment.syntaxKind = MdSyntaxKind::Comment;
            flow.AppendText(line.substr(index), comment);
            return;
        }
        if (line[index] == '"' || line[index] == '\'') {
            const char quote = line[index];
            size_t end = index + 1;
            while (end < line.size()) {
                if (line[end] == '\\' && end + 1 < line.size()) {
                    end += 2;
                    continue;
                }
                ++end;
                if (line[end - 1] == quote) {
                    break;
                }
            }
            Style stringStyle = style;
            stringStyle.syntaxKind = MdSyntaxKind::String;
            flow.AppendText(line.substr(index, end - index), stringStyle);
            index = end;
            continue;
        }
        if (std::isdigit(current)) {
            size_t end = index + 1;
            while (end < line.size() && (std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '.' || line[end] == '_')) {
                ++end;
            }
            Style numberStyle = style;
            numberStyle.syntaxKind = MdSyntaxKind::Number;
            flow.AppendText(line.substr(index, end - index), numberStyle);
            index = end;
            continue;
        }
        if (IsIdentifierStart(current)) {
            size_t end = index + 1;
            while (end < line.size() && IsIdentifierPart(static_cast<unsigned char>(line[end]))) {
                ++end;
            }
            const std::string token = line.substr(index, end - index);
            Style tokenStyle = style;
            const std::string normalized = ToLowerAscii(token);
            if (IsKeyword(lang, normalized)) {
                tokenStyle.syntaxKind = MdSyntaxKind::Keyword;
            } else if (IsTypeToken(token)) {
                tokenStyle.syntaxKind = MdSyntaxKind::Type;
            }
            flow.AppendText(token, tokenStyle);
            index = end;
            continue;
        }
        Style punctuation = style;
        if (!python && !json && index == 0 && line[index] == '#') {
            punctuation.syntaxKind = MdSyntaxKind::Preprocessor;
        }
        flow.AppendText(line.substr(index, 1), punctuation);
        ++index;
    }
}
float HeadingSize(int level) {
    switch (level) {
    case 1: return 28.0f;
    case 2: return 22.0f;
    case 3: return 18.5f;
    case 4: return 16.0f;
    case 5: return 14.5f;
    default: return 13.5f;
    }
}

void LayoutBlocks(
    GraphicsContext& ctx,
    const std::vector<MdBlock>& blocks,
    float width,
    float x0,
    float& y,
    MdLayoutResult& out,
    bool showLineNumbers);

void LayoutFlowBlock(
    GraphicsContext& ctx,
    const std::vector<MdInline>& inlines,
    float x0,
    float width,
    float& y,
    float size,
    DWRITE_FONT_WEIGHT weight,
    int headingLevel,
    MdLayoutResult& out) {
    Flow flow{ ctx, out, x0, x0 + width, x0, y, size * 1.55f, y, {} };
    Style style;
    style.size = size;
    style.weight = weight;
    const size_t runBegin = out.runs.size();
    flow.AppendInlines(inlines, style);
    y = flow.y + flow.lineH;
    out.plain.push_back('\n');
    MdPaintBlock pb;
    pb.type = MdPaintBlock::Type::Flow;
    pb.headingLevel = headingLevel;
    float minX = x0;
    float maxX = x0;
    float minY = flow.lineStartY;
    float maxY = y;
    for (size_t i = runBegin; i < out.runs.size(); ++i) {
        pb.runs.push_back(out.runs[i]);
        minX = (std::min)(minX, out.runs[i].bounds.x);
        maxX = (std::max)(maxX, out.runs[i].bounds.x + out.runs[i].bounds.width);
        minY = (std::min)(minY, out.runs[i].bounds.y);
        maxY = (std::max)(maxY, out.runs[i].bounds.y + out.runs[i].bounds.height);
    }
    pb.bounds = Rect(x0, minY, width, (std::max)(4.0f, maxY - minY));
    out.blocks.push_back(std::move(pb));
}

void LayoutBlocks(
    GraphicsContext& ctx,
    const std::vector<MdBlock>& blocks,
    float width,
    float x0,
    float& y,
    MdLayoutResult& out,
    bool showLineNumbers) {
    for (const auto& block : blocks) {
        switch (block.kind) {
        case MdBlockKind::Paragraph:
            LayoutFlowBlock(ctx, block.inlines, x0, width, y, 14.0f, DWRITE_FONT_WEIGHT_NORMAL, 0, out);
            y += 8.0f;
            break;
        case MdBlockKind::Heading: {
            const float sz = HeadingSize(block.headingLevel);
            if (block.headingLevel <= 2 && y > 4.0f) {
                y += 6.0f;
            }
            LayoutFlowBlock(ctx, block.inlines, x0, width, y, sz, DWRITE_FONT_WEIGHT_SEMI_BOLD, block.headingLevel, out);
            y += block.headingLevel <= 2 ? 12.0f : 8.0f;
            break;
        }
        case MdBlockKind::ThematicBreak: {
            MdPaintBlock pb;
            pb.type = MdPaintBlock::Type::Hr;
            pb.bounds = Rect(x0, y + 8.0f, width, 12.0f);
            out.blocks.push_back(pb);
            out.plain += "\n";
            y += 20.0f;
            break;
        }
        case MdBlockKind::Quote: {
            const float startY = y;
            LayoutBlocks(ctx, block.children, (std::max)(40.0f, width - 16.0f), x0 + 14.0f, y, out, showLineNumbers);
            MdPaintBlock pb;
            pb.type = MdPaintBlock::Type::Quote;
            pb.bounds = Rect(x0, startY, width, (std::max)(12.0f, y - startY));
            out.blocks.push_back(pb);
            y += 8.0f;
            break;
        }
        case MdBlockKind::List: {
            int index = block.listStart;
            for (const auto& item : block.children) {
                const float indent = 8.0f + static_cast<float>(item.listLevel) * 18.0f;
                Style markerStyle;
                markerStyle.size = 14.0f;
                markerStyle.weight = DWRITE_FONT_WEIGHT_NORMAL;
                const std::string marker = item.isTask
                    ? (item.isTaskChecked ? "☑" : "☐")
                    : (item.ordered ? (std::to_string(index++) + ".") : "•");
                const Size ms = MeasureStyled(ctx, marker, markerStyle);
                MdRun mark;
                mark.bounds = Rect(x0 + indent, y, ms.width, 20.0f);
                mark.text = marker;
                mark.size = 14.0f;
                mark.plainStart = static_cast<int>(out.plain.size());
                out.plain += marker;
                out.plain.push_back(' ');
                mark.plainEnd = static_cast<int>(out.plain.size());
                out.runs.push_back(mark);
                LayoutFlowBlock(
                    ctx,
                    item.inlines,
                    x0 + indent + ms.width + 8.0f,
                    (std::max)(40.0f, width - indent - ms.width - 8.0f),
                    y,
                    14.0f,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    0,
                    out);
                y += 2.0f;
            }
            y += 6.0f;
            break;
        }
        case MdBlockKind::Code: {
            std::vector<std::string> lines;
            std::stringstream ss(block.literal);
            std::string line;
            if (block.literal.empty()) {
                lines.emplace_back();
            } else {
                while (std::getline(ss, line)) {
                    lines.push_back(line);
                }
            }
            const float gutter = showLineNumbers ? 36.0f : 10.0f;
            const float codeSize = 12.5f;
            const float rowH = 20.0f;
            const float padTop = 30.0f;
            const float padBot = 10.0f;
            const float blockTop = y;
            MdPaintBlock pb;
            pb.type = MdPaintBlock::Type::Code;
            pb.codeLines = lines;
            pb.codeLanguage = block.lang;
            Style cs;
            cs.font = "Consolas";
            cs.size = codeSize;
            float cy = y + padTop;
            int lineNo = 1;
            for (const auto& codeLine : lines) {
                if (showLineNumbers) {
                    MdRun num;
                    const std::string ns = std::to_string(lineNo);
                    const Size nsSz = MeasureStyled(ctx, ns, cs);
                    num.bounds = Rect(x0 + 8.0f, cy, nsSz.width, rowH);
                    num.text = ns;
                    num.font = "Consolas";
                    num.size = 11.0f;
                    num.code = true;
                    num.syntaxKind = MdSyntaxKind::Comment;
                    pb.runs.push_back(num);
                }
                Flow flow{ ctx, out, x0 + gutter, x0 + width - 10.0f, x0 + gutter, cy, rowH, cy, {} };
                const size_t begin = out.runs.size();
                AppendHighlightedCode(flow, codeLine.empty() ? " " : codeLine, block.lang, cs);
                out.plain.push_back('\n');
                for (size_t runIndex = begin; runIndex < out.runs.size(); ++runIndex) {
                    pb.runs.push_back(out.runs[runIndex]);
                }
                cy = flow.y + rowH;
                ++lineNo;
            }
            pb.bounds = Rect(x0, blockTop, width, cy - blockTop + padBot);
            out.blocks.push_back(std::move(pb));
            y += out.blocks.back().bounds.height + 12.0f;
            break;
        }
        case MdBlockKind::Table: {
            if (block.tableRows.empty()) {
                break;
            }
            int cols = 0;
            for (const auto& row : block.tableRows) {
                cols = (std::max)(cols, static_cast<int>(row.size()));
            }
            cols = (std::max)(cols, 1);
            const float colW = width / static_cast<float>(cols);
            MdPaintBlock pb;
            pb.type = MdPaintBlock::Type::Table;
            pb.tableCols = cols;
            pb.tableRows = static_cast<int>(block.tableRows.size());
            const float tableTop = y;
            for (int rowIndex = 0; rowIndex < pb.tableRows; ++rowIndex) {
                const float rowTop = y;
                float rowHeight = 28.0f;
                for (int columnIndex = 0; columnIndex < cols; ++columnIndex) {
                    const std::string cell = (columnIndex < static_cast<int>(block.tableRows[static_cast<size_t>(rowIndex)].size()))
                        ? block.tableRows[static_cast<size_t>(rowIndex)][static_cast<size_t>(columnIndex)]
                        : "";
                    Style style;
                    style.size = 13.0f;
                    style.weight = rowIndex == 0 ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
                    const float cellTop = rowTop + 6.0f;
                    Flow flow{
                        ctx,
                        out,
                        x0 + colW * static_cast<float>(columnIndex) + 8.0f,
                        x0 + colW * static_cast<float>(columnIndex + 1) - 8.0f,
                        x0 + colW * static_cast<float>(columnIndex) + 8.0f,
                        cellTop,
                        20.0f,
                        cellTop,
                        {}
                    };
                    const size_t begin = out.runs.size();
                    flow.AppendInlines(ParseMarkdownInlines(cell), style);
                    for (size_t runIndex = begin; runIndex < out.runs.size(); ++runIndex) {
                        pb.runs.push_back(out.runs[runIndex]);
                    }
                    rowHeight = (std::max)(rowHeight, flow.y - cellTop + flow.lineH + 12.0f);
                    out.plain.push_back('\t');
                }
                out.plain.push_back('\n');
                pb.tableRowHeights.push_back(rowHeight);
                y += rowHeight;
            }
            pb.bounds = Rect(x0, tableTop, width, y - tableTop);
            out.blocks.push_back(std::move(pb));
            y += 12.0f;
            break;
        }
        default:
            break;
        }
    }
}

} // namespace

MdLayoutResult LayoutMarkdown(
    GraphicsContext& ctx,
    const std::vector<MdBlock>& blocks,
    float width,
    bool showCodeLineNumbers) {
    MdLayoutResult out;
    out.width = width;
    float y = 0.0f;
    LayoutBlocks(ctx, blocks, width, 0.0f, y, out, showCodeLineNumbers);
    out.height = y + 8.0f;
    return out;
}

} // namespace CUI
