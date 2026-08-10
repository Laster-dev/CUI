#pragma once
#include <string>

namespace CUI {
namespace Term {

// Encoding helpers shared by the engine and the renderer, so terminal/ stays
// self-contained instead of depending on the wider framework core.
std::string Utf8FromUtf16(const std::wstring& text);
std::wstring Utf16FromUtf8(const std::string& text);
// Appends one code point (surrogate pair aware) to a UTF-8 string.
void AppendUtf8CodePoint(std::string& out, int codePoint);
// Appends one code point to a UTF-16 string.
void AppendUtf16CodePoint(std::wstring& out, int codePoint);

// Terminal cell width (wcwidth-like). Tuned for VT / Oh My Posh:
// Powerline PUA and box-drawing stay width 1; only real emoji are width 2.
class UnicodeWidth {
public:
    static int GetWidth(int codePoint);

    struct Cluster {
        int width = 0;
        int nextIndex = 0;
    };

    // Advance over one grapheme-ish cluster starting at index in a UTF-16 string.
    static Cluster MeasureCluster(const std::wstring& s, int index);

    static bool IsCombining(int cp);
    static bool IsZeroWidth(int cp);

private:
    static bool IsEmojiPresentation(int cp);
    static bool IsWide(int cp);
};

} // namespace Term
} // namespace CUI
