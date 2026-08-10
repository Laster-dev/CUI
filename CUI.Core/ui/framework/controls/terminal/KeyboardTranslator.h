#pragma once
#include "MouseReporter.h" // TermModifiers
#include <string>

namespace CUI {
namespace Term {

// Maps Win32 virtual-key events to VT sequences (xterm-compatible).
// Printable keys are intentionally left unhandled so the host can route them
// through WM_CHAR / FromTextInput instead.
class KeyboardTranslator {
public:
    // Returns true when vkCode produced a sequence (which may contain NUL bytes).
    static bool Translate(int vkCode, unsigned mods, bool applicationCursor, std::string& out);

    // Wraps pasted/composed text in bracketed-paste markers when the app asked for them.
    static bool FromTextInput(const std::string& text, bool bracketedPaste, std::string& out);

private:
    static std::string Arrow(const char* letter, bool app, bool ctrl, bool alt, bool shift);
    static std::string ModSeq(const char* num, const char* tilde, const char* appLetter,
                             bool app, bool ctrl, bool alt, bool shift);
    static std::string CsiNum(const char* num, bool ctrl, bool alt, bool shift);
    static int ModifierCode(bool ctrl, bool alt, bool shift);
};

} // namespace Term
} // namespace CUI
