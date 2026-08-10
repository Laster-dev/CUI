#include "UnicodeWidth.h"
#include <windows.h>

namespace CUI {
namespace Term {

std::string Utf8FromUtf16(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }
    const int needed = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                           nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return std::string();
    }
    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                        &out[0], needed, nullptr, nullptr);
    return out;
}

std::wstring Utf16FromUtf8(const std::string& text) {
    if (text.empty()) {
        return std::wstring();
    }
    const int needed = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                           nullptr, 0);
    if (needed <= 0) {
        return std::wstring();
    }
    std::wstring out(static_cast<size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &out[0], needed);
    return out;
}

void AppendUtf8CodePoint(std::string& out, int codePoint) {
    if (codePoint < 0 || codePoint > 0x10FFFF) {
        codePoint = 0xFFFD;
    }
    if (codePoint < 0x80) {
        out.push_back(static_cast<char>(codePoint));
    } else if (codePoint < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

void AppendUtf16CodePoint(std::wstring& out, int codePoint) {
    if (codePoint < 0 || codePoint > 0x10FFFF) {
        codePoint = 0xFFFD;
    }
    if (codePoint <= 0xFFFF) {
        out.push_back(static_cast<wchar_t>(codePoint));
        return;
    }
    const int v = codePoint - 0x10000;
    out.push_back(static_cast<wchar_t>(0xD800 + (v >> 10)));
    out.push_back(static_cast<wchar_t>(0xDC00 + (v & 0x3FF)));
}

namespace {
bool InRange(int cp, int lo, int hi) {
    return cp >= lo && cp <= hi;
}

// Decode one UTF-16 code point at index (surrogate aware), matching char.ConvertToUtf32.
int ConvertToUtf32(const std::wstring& s, int index) {
    const wchar_t c = s[static_cast<size_t>(index)];
    if (c >= 0xD800 && c <= 0xDBFF && static_cast<size_t>(index) + 1 < s.size()) {
        const wchar_t low = s[static_cast<size_t>(index) + 1];
        if (low >= 0xDC00 && low <= 0xDFFF) {
            return 0x10000 + ((static_cast<int>(c) - 0xD800) << 10) + (static_cast<int>(low) - 0xDC00);
        }
    }
    return static_cast<int>(c);
}
}

int UnicodeWidth::GetWidth(int codePoint) {
    if (codePoint == 0) {
        return 0;
    }

    if (IsCombining(codePoint) || IsZeroWidth(codePoint)) {
        return 0;
    }

    if (codePoint < 0x20 || InRange(codePoint, 0x7F, 0x9F)) {
        return 0;
    }

    // Powerline / Nerd Font Private Use - always one cell (match WT / xterm).
    if (InRange(codePoint, 0xE000, 0xF8FF)) {
        return 1;
    }
    if (InRange(codePoint, 0xF0000, 0xFFFFD)) {
        return 1;
    }

    // Box drawing, block elements, geometric shapes used by prompts - width 1.
    if (InRange(codePoint, 0x2500, 0x259F)) {
        return 1;
    }
    if (InRange(codePoint, 0x25A0, 0x25FF)) {
        return 1;
    }

    if (codePoint < 0x1100) {
        return 1;
    }

    if (IsWide(codePoint) || IsEmojiPresentation(codePoint)) {
        return 2;
    }

    return 1;
}

UnicodeWidth::Cluster UnicodeWidth::MeasureCluster(const std::wstring& s, int index) {
    Cluster result;
    const int len = static_cast<int>(s.size());
    if (index >= len) {
        result.width = 0;
        result.nextIndex = index;
        return result;
    }

    const int cp = ConvertToUtf32(s, index);
    int next = index + (cp > 0xFFFF ? 2 : 1);
    int width = GetWidth(cp);

    while (next < len) {
        const int ncp = ConvertToUtf32(s, next);
        if (ncp == 0x200D) {
            next += 1;
            if (next >= len) {
                break;
            }
            const int join = ConvertToUtf32(s, next);
            next += join > 0xFFFF ? 2 : 1;
            if (width < 2 && IsEmojiPresentation(join)) {
                width = 2;
            }
            continue;
        }
        if (IsZeroWidth(ncp) || IsCombining(ncp)) {
            next += ncp > 0xFFFF ? 2 : 1;
            continue;
        }
        break;
    }

    if (width <= 0 && cp != 0) {
        width = 0;
    }

    result.width = width;
    result.nextIndex = next;
    return result;
}

bool UnicodeWidth::IsCombining(int cp) {
    return InRange(cp, 0x0300, 0x036F)
        || InRange(cp, 0x0483, 0x0489)
        || InRange(cp, 0x0591, 0x05BD)
        || InRange(cp, 0x0610, 0x061A)
        || InRange(cp, 0x064B, 0x065F)
        || InRange(cp, 0x07A6, 0x07B0)
        || InRange(cp, 0x0E31, 0x0E3A)
        || InRange(cp, 0x0E47, 0x0E4E)
        || InRange(cp, 0x1AB0, 0x1AFF)
        || InRange(cp, 0x1DC0, 0x1DFF)
        || InRange(cp, 0x20D0, 0x20FF)
        || InRange(cp, 0xFE20, 0xFE2F);
}

bool UnicodeWidth::IsZeroWidth(int cp) {
    return cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0x2060 || cp == 0xFEFF
        || InRange(cp, 0xFE00, 0xFE0F)
        || InRange(cp, 0x1F3FB, 0x1F3FF)
        || InRange(cp, 0xE0100, 0xE01EF);
}

bool UnicodeWidth::IsEmojiPresentation(int cp) {
    return InRange(cp, 0x1F300, 0x1FAFF)
        || InRange(cp, 0x1F000, 0x1F2FF)
        || InRange(cp, 0x1F600, 0x1F64F)
        || InRange(cp, 0x1F680, 0x1F6FF)
        // Regional indicator pairs base (flags handled loosely as 2)
        || InRange(cp, 0x1F1E6, 0x1F1FF);
}

bool UnicodeWidth::IsWide(int cp) {
    return InRange(cp, 0x1100, 0x115F)
        || InRange(cp, 0x2329, 0x232A)
        || InRange(cp, 0x2E80, 0xA4CF)
        || InRange(cp, 0xAC00, 0xD7A3)
        || InRange(cp, 0xF900, 0xFAFF)
        || InRange(cp, 0xFE10, 0xFE19)
        || InRange(cp, 0xFE30, 0xFE6F)
        || InRange(cp, 0xFF00, 0xFF60)
        || InRange(cp, 0xFFE0, 0xFFE6)
        || InRange(cp, 0x20000, 0x3FFFD);
}

} // namespace Term
} // namespace CUI
