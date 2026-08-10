#include "KeyboardTranslator.h"
#include <windows.h>

namespace CUI {
namespace Term {

bool KeyboardTranslator::Translate(int vkCode, unsigned mods, bool applicationCursor, std::string& out) {
    const bool ctrl = (mods & ModControl) != 0;
    const bool alt = (mods & ModAlt) != 0;
    const bool shift = (mods & ModShift) != 0;

    switch (vkCode) {
    case VK_RETURN:
        out = "\r";
        return true;
    case VK_TAB:
        out = shift ? "\x1b[Z" : "\t";
        return true;
    case VK_BACK:
        out = "\x7f";
        return true;
    case VK_ESCAPE:
        out = "\x1b";
        return true;
    case VK_SPACE:
        if (ctrl) {
            out = std::string(1, '\0');
            return true;
        }
        return false;
    case VK_UP:
        out = Arrow("A", applicationCursor, ctrl, alt, shift);
        return true;
    case VK_DOWN:
        out = Arrow("B", applicationCursor, ctrl, alt, shift);
        return true;
    case VK_RIGHT:
        out = Arrow("C", applicationCursor, ctrl, alt, shift);
        return true;
    case VK_LEFT:
        out = Arrow("D", applicationCursor, ctrl, alt, shift);
        return true;
    case VK_HOME:
        out = ModSeq("1", "~", "H", applicationCursor, ctrl, alt, shift);
        return true;
    case VK_END:
        out = ModSeq("1", "~", "F", applicationCursor, ctrl, alt, shift);
        return true;
    case VK_INSERT:
        out = CsiNum("2", ctrl, alt, shift);
        return true;
    case VK_DELETE:
        out = CsiNum("3", ctrl, alt, shift);
        return true;
    case VK_PRIOR:
        out = CsiNum("5", ctrl, alt, shift);
        return true;
    case VK_NEXT:
        out = CsiNum("6", ctrl, alt, shift);
        return true;
    case VK_F1: out = "\x1bOP"; return true;
    case VK_F2: out = "\x1bOQ"; return true;
    case VK_F3: out = "\x1bOR"; return true;
    case VK_F4: out = "\x1bOS"; return true;
    case VK_F5: out = "\x1b[15~"; return true;
    case VK_F6: out = "\x1b[17~"; return true;
    case VK_F7: out = "\x1b[18~"; return true;
    case VK_F8: out = "\x1b[19~"; return true;
    case VK_F9: out = "\x1b[20~"; return true;
    case VK_F10: out = "\x1b[21~"; return true;
    case VK_F11: out = "\x1b[23~"; return true;
    case VK_F12: out = "\x1b[24~"; return true;
    default:
        break;
    }

    if (ctrl && vkCode >= 'A' && vkCode <= 'Z') {
        out = std::string(1, static_cast<char>(vkCode - 'A' + 1));
        return true;
    }

    // Ctrl+@, [, \, ], ^, _
    if (ctrl) {
        switch (vkCode) {
        case VK_OEM_2:
            if (shift) { out = "\x1f"; return true; } // Ctrl+_
            return false;
        case VK_OEM_4: out = "\x1b"; return true; // Ctrl+[
        case VK_OEM_5: out = "\x1c"; return true; // Ctrl+backslash
        case VK_OEM_6: out = "\x1d"; return true; // Ctrl+]
        case '6':
            if (shift) { out = "\x1e"; return true; } // Ctrl+^
            return false;
        case VK_OEM_3:
            if (!shift) { out = std::string(1, '\0'); return true; } // Ctrl+@
            return false;
        default:
            return false;
        }
    }

    return false;
}

bool KeyboardTranslator::FromTextInput(const std::string& text, bool bracketedPaste, std::string& out) {
    if (text.empty()) {
        return false;
    }

    if (text.size() > 1 && bracketedPaste) {
        out = "\x1b[200~" + text + "\x1b[201~";
        return true;
    }

    out = text;
    return true;
}

std::string KeyboardTranslator::Arrow(const char* letter, bool app, bool ctrl, bool alt, bool shift) {
    const int mod = ModifierCode(ctrl, alt, shift);
    if (mod > 1) {
        return std::string("\x1b[1;") + std::to_string(mod) + letter;
    }
    return app ? std::string("\x1bO") + letter : std::string("\x1b[") + letter;
}

std::string KeyboardTranslator::ModSeq(const char* num, const char* tilde, const char* appLetter,
                                      bool app, bool ctrl, bool alt, bool shift) {
    const int mod = ModifierCode(ctrl, alt, shift);
    if (app && mod == 1) {
        return std::string("\x1bO") + appLetter;
    }
    if (mod > 1) {
        return std::string("\x1b[") + num + ";" + std::to_string(mod) + tilde;
    }
    const std::string letter(appLetter);
    if (letter == "H" || letter == "F") {
        return std::string("\x1b[") + letter;
    }
    return std::string("\x1b[") + num + tilde;
}

std::string KeyboardTranslator::CsiNum(const char* num, bool ctrl, bool alt, bool shift) {
    const int mod = ModifierCode(ctrl, alt, shift);
    return mod > 1
        ? std::string("\x1b[") + num + ";" + std::to_string(mod) + "~"
        : std::string("\x1b[") + num + "~";
}

int KeyboardTranslator::ModifierCode(bool ctrl, bool alt, bool shift) {
    int mod = 1;
    if (shift) mod += 1;
    if (alt) mod += 2;
    if (ctrl) mod += 4;
    return mod;
}

} // namespace Term
} // namespace CUI
