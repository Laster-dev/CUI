#include "MarkdownAst.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace CUI {
namespace {

std::string Trim(const std::string& s) {
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) {
        ++a;
    }
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) {
        --b;
    }
    return s.substr(a, b - a);
}

int LeadingSpaces(const std::string& s) {
    int n = 0;
    while (n < static_cast<int>(s.size()) && s[static_cast<size_t>(n)] == ' ') {
        ++n;
    }
    return n;
}

std::vector<std::string> SplitLines(const std::string& src) {
    std::vector<std::string> lines;
    std::string cur;
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '\r') {
            continue;
        }
        if (src[i] == '\n') {
            lines.push_back(std::move(cur));
            cur.clear();
        } else {
            cur.push_back(src[i]);
        }
    }
    lines.push_back(std::move(cur));
    return lines;
}

bool IsBlank(const std::string& s) {
    return Trim(s).empty();
}

bool IsHr(const std::string& line) {
    const std::string t = Trim(line);
    if (t.size() < 3) {
        return false;
    }
    char c = t[0];
    if (c != '-' && c != '*' && c != '_') {
        return false;
    }
    int count = 0;
    for (char ch : t) {
        if (ch == c) {
            ++count;
        } else if (!std::isspace(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return count >= 3;
}

int HeadingLevel(const std::string& line, std::string& title) {
    size_t i = 0;
    while (i < line.size() && line[i] == ' ') {
        ++i;
    }
    int level = 0;
    while (i < line.size() && line[i] == '#' && level < 6) {
        ++level;
        ++i;
    }
    if (level == 0 || i >= line.size() || (line[i] != ' ' && line[i] != '\t')) {
        return 0;
    }
    title = Trim(line.substr(i));
    while (!title.empty() && title.back() == '#') {
        title.pop_back();
    }
    title = Trim(title);
    return level;
}

bool FenceOpen(const std::string& line, std::string& lang) {
    std::string t = Trim(line);
    if (t.size() < 3 || t.substr(0, 3) != "```") {
        return false;
    }
    lang = Trim(t.substr(3));
    return true;
}

bool FenceClose(const std::string& line) {
    std::string t = Trim(line);
    return t.size() >= 3 && t.substr(0, 3) == "```";
}

bool ParseListMarker(const std::string& line, int& level, bool& ordered, int& start, std::string& rest) {
    int spaces = LeadingSpaces(line);
    if (spaces >= 8) {
        return false;
    }
    size_t i = static_cast<size_t>(spaces);
    if (i >= line.size()) {
        return false;
    }
    ordered = false;
    start = 1;
    if (line[i] == '-' || line[i] == '*' || line[i] == '+') {
        if (i + 1 >= line.size() || (line[i + 1] != ' ' && line[i + 1] != '\t')) {
            return false;
        }
        rest = Trim(line.substr(i + 2));
        level = spaces / 2;
        return true;
    }
    size_t j = i;
    if (!std::isdigit(static_cast<unsigned char>(line[j]))) {
        return false;
    }
    int num = 0;
    while (j < line.size() && std::isdigit(static_cast<unsigned char>(line[j]))) {
        num = num * 10 + (line[j] - '0');
        ++j;
    }
    if (j >= line.size() || (line[j] != '.' && line[j] != ')')) {
        return false;
    }
    ++j;
    if (j >= line.size() || (line[j] != ' ' && line[j] != '\t')) {
        return false;
    }
    ordered = true;
    start = num;
    rest = Trim(line.substr(j + 1));
    level = spaces / 2;
    return true;
}

bool LooksLikeTableRow(const std::string& line) {
    const std::string t = Trim(line);
    return t.find('|') != std::string::npos;
}

bool IsTableSep(const std::string& line, std::vector<int>& align) {
    std::string t = Trim(line);
    if (t.empty() || t.find('|') == std::string::npos) {
        return false;
    }
    if (t.front() == '|') {
        t.erase(t.begin());
    }
    if (!t.empty() && t.back() == '|') {
        t.pop_back();
    }
    align.clear();
    std::stringstream ss(t);
    std::string cell;
    int cols = 0;
    while (std::getline(ss, cell, '|')) {
        std::string c = Trim(cell);
        if (c.empty()) {
            continue;
        }
        bool colonL = !c.empty() && c.front() == ':';
        bool colonR = !c.empty() && c.back() == ':';
        for (char ch : c) {
            if (ch != '-' && ch != ':') {
                return false;
            }
        }
        if (colonL && colonR) {
            align.push_back(0);
        } else if (colonR) {
            align.push_back(1);
        } else {
            align.push_back(-1);
        }
        ++cols;
    }
    return cols >= 1;
}

std::vector<std::string> SplitTableRow(const std::string& line) {
    std::string t = Trim(line);
    if (!t.empty() && t.front() == '|') {
        t.erase(t.begin());
    }
    if (!t.empty() && t.back() == '|') {
        t.pop_back();
    }
    std::vector<std::string> cells;
    std::string cur;
    for (size_t i = 0; i < t.size(); ++i) {
        if (t[i] == '\\' && i + 1 < t.size() && t[i + 1] == '|') {
            cur.push_back('|');
            ++i;
        } else if (t[i] == '|') {
            cells.push_back(Trim(cur));
            cur.clear();
        } else {
            cur.push_back(t[i]);
        }
    }
    cells.push_back(Trim(cur));
    return cells;
}

size_t FindClosing(const std::string& s, size_t from, const std::string& delim) {
    for (size_t i = from; i + delim.size() <= s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            continue;
        }
        if (s.compare(i, delim.size(), delim) == 0) {
            return i;
        }
    }
    return std::string::npos;
}

void AppendText(std::vector<MdInline>& out, std::string text) {
    if (text.empty()) {
        return;
    }
    if (!out.empty() && out.back().kind == MdInlineKind::Text) {
        out.back().text += text;
        return;
    }
    MdInline n;
    n.kind = MdInlineKind::Text;
    n.text = std::move(text);
    out.push_back(std::move(n));
}

std::vector<MdInline> ParseInlines(const std::string& s) {
    std::vector<MdInline> out;
    std::string acc;
    size_t i = 0;
    auto flush = [&]() {
        AppendText(out, acc);
        acc.clear();
    };
    while (i < s.size()) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            acc.push_back(s[i + 1]);
            i += 2;
            continue;
        }
        if (s.compare(i, 2, "**") == 0 || s.compare(i, 2, "__") == 0) {
            const std::string delim = s.substr(i, 2);
            const size_t close = FindClosing(s, i + 2, delim);
            if (close != std::string::npos) {
                flush();
                MdInline n;
                n.kind = MdInlineKind::Strong;
                n.children = ParseInlines(s.substr(i + 2, close - (i + 2)));
                out.push_back(std::move(n));
                i = close + 2;
                continue;
            }
        }
        if (s[i] == '*' || s[i] == '_') {
            const std::string delim(1, s[i]);
            const size_t close = FindClosing(s, i + 1, delim);
            if (close != std::string::npos && close > i + 1) {
                flush();
                MdInline n;
                n.kind = MdInlineKind::Emphasis;
                n.children = ParseInlines(s.substr(i + 1, close - (i + 1)));
                out.push_back(std::move(n));
                i = close + 1;
                continue;
            }
        }
        if (s[i] == '`') {
            size_t ticks = 1;
            while (i + ticks < s.size() && s[i + ticks] == '`') {
                ++ticks;
            }
            const std::string delim(ticks, '`');
            const size_t close = s.find(delim, i + ticks);
            if (close != std::string::npos) {
                flush();
                MdInline n;
                n.kind = MdInlineKind::Code;
                n.text = s.substr(i + ticks, close - (i + ticks));
                out.push_back(std::move(n));
                i = close + ticks;
                continue;
            }
        }
        if (s[i] == '[') {
            const size_t close = FindClosing(s, i + 1, "]");
            if (close != std::string::npos && close + 1 < s.size() && s[close + 1] == '(') {
                const size_t urlEnd = FindClosing(s, close + 2, ")");
                if (urlEnd != std::string::npos) {
                    flush();
                    MdInline n;
                    n.kind = MdInlineKind::Link;
                    n.children = ParseInlines(s.substr(i + 1, close - (i + 1)));
                    n.href = Trim(s.substr(close + 2, urlEnd - (close + 2)));
                    const auto sp = n.href.find(' ');
                    if (sp != std::string::npos) {
                        n.href = Trim(n.href.substr(0, sp));
                    }
                    out.push_back(std::move(n));
                    i = urlEnd + 1;
                    continue;
                }
            }
        }
        acc.push_back(s[i]);
        ++i;
    }
    flush();
    return out;
}

} // namespace

std::vector<MdInline> ParseMarkdownInlines(const std::string& source) {
    return ParseInlines(source);
}

std::vector<MdBlock> ParseMarkdown(const std::string& source) {
    const auto lines = SplitLines(source);
    std::vector<MdBlock> blocks;
    size_t i = 0;

    auto flushPara = [&](std::string& para) {
        para = Trim(para);
        if (para.empty()) {
            return;
        }
        MdBlock b;
        b.kind = MdBlockKind::Paragraph;
        b.inlines = ParseMarkdownInlines(para);
        blocks.push_back(std::move(b));
        para.clear();
    };

    std::string para;
    while (i < lines.size()) {
        const std::string& line = lines[i];
        if (IsBlank(line)) {
            flushPara(para);
            ++i;
            continue;
        }

        std::string lang;
        if (FenceOpen(line, lang)) {
            flushPara(para);
            MdBlock b;
            b.kind = MdBlockKind::Code;
            b.lang = lang;
            ++i;
            std::ostringstream body;
            bool first = true;
            while (i < lines.size() && !FenceClose(lines[i])) {
                if (!first) {
                    body << '\n';
                }
                body << lines[i];
                first = false;
                ++i;
            }
            if (i < lines.size()) {
                ++i;
            }
            b.literal = body.str();
            blocks.push_back(std::move(b));
            continue;
        }

        std::string title;
        if (const int level = HeadingLevel(line, title)) {
            flushPara(para);
            MdBlock b;
            b.kind = MdBlockKind::Heading;
            b.headingLevel = level;
            b.inlines = ParseMarkdownInlines(title);
            blocks.push_back(std::move(b));
            ++i;
            continue;
        }

        if (IsHr(line)) {
            flushPara(para);
            MdBlock b;
            b.kind = MdBlockKind::ThematicBreak;
            blocks.push_back(std::move(b));
            ++i;
            continue;
        }

        if (LooksLikeTableRow(line) && i + 1 < lines.size()) {
            std::vector<int> align;
            if (IsTableSep(lines[i + 1], align)) {
                flushPara(para);
                MdBlock b;
                b.kind = MdBlockKind::Table;
                b.tableAlign = align;
                b.tableRows.push_back(SplitTableRow(line));
                i += 2;
                while (i < lines.size() && LooksLikeTableRow(lines[i]) && !IsBlank(lines[i])) {
                    b.tableRows.push_back(SplitTableRow(lines[i]));
                    ++i;
                }
                blocks.push_back(std::move(b));
                continue;
            }
        }

        if (!line.empty() && Trim(line)[0] == '>') {
            flushPara(para);
            MdBlock quote;
            quote.kind = MdBlockKind::Quote;
            std::string inner;
            while (i < lines.size()) {
                const std::string t = Trim(lines[i]);
                if (t.empty()) {
                    break;
                }
                if (t[0] != '>') {
                    break;
                }
                std::string rest = t.substr(1);
                if (!rest.empty() && rest[0] == ' ') {
                    rest.erase(rest.begin());
                }
                if (!inner.empty()) {
                    inner.push_back('\n');
                }
                inner += rest;
                ++i;
            }
            auto nested = ParseMarkdown(inner);
            if (nested.empty()) {
                MdBlock p;
                p.kind = MdBlockKind::Paragraph;
                p.inlines = ParseMarkdownInlines(inner);
                quote.children.push_back(std::move(p));
            } else {
                quote.children = std::move(nested);
            }
            blocks.push_back(std::move(quote));
            continue;
        }

        int listLevel = 0;
        bool ordered = false;
        int start = 1;
        std::string rest;
        if (ParseListMarker(line, listLevel, ordered, start, rest)) {
            flushPara(para);
            MdBlock list;
            list.kind = MdBlockKind::List;
            list.ordered = ordered;
            list.listStart = start;
            while (i < lines.size()) {
                int lvl = 0;
                bool ord = ordered;
                int st = 1;
                std::string itemText;
                if (!ParseListMarker(lines[i], lvl, ord, st, itemText)) {
                    if (!IsBlank(lines[i]) && LeadingSpaces(lines[i]) >= 2) {
                        if (!list.children.empty()) {
                            auto& last = list.children.back();
                            MdInline br;
                            br.kind = MdInlineKind::Break;
                            last.inlines.push_back(br);
                            auto more = ParseMarkdownInlines(Trim(lines[i]));
                            last.inlines.insert(last.inlines.end(), more.begin(), more.end());
                        }
                        ++i;
                        continue;
                    }
                    break;
                }
                MdBlock item;
                item.kind = MdBlockKind::ListItem;
                item.ordered = ord;
                item.listLevel = lvl;
                item.listStart = st;
                item.inlines = ParseMarkdownInlines(itemText);
                list.children.push_back(std::move(item));
                ++i;
            }
            blocks.push_back(std::move(list));
            continue;
        }

        if (!para.empty()) {
            para.push_back('\n');
        }
        para += Trim(line);
        ++i;
    }
    flushPara(para);
    return blocks;
}

} // namespace CUI
