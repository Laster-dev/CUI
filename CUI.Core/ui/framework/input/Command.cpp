#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Command.h"
#include <windows.h>
#include <algorithm>
#include <cctype>

namespace CUI {
namespace {

std::string ToLowerCopy(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::vector<std::string> SplitPlus(const std::string& text) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : text) {
        if (c == '+') {
            if (!cur.empty()) {
                parts.push_back(cur);
                cur.clear();
            }
        } else if (c != ' ' && c != '\t') {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) {
        parts.push_back(cur);
    }
    return parts;
}

int VirtualKeyFromToken(const std::string& token) {
    const std::string t = ToLowerCopy(token);
    if (t.size() == 1) {
        const char c = t[0];
        if (c >= 'a' && c <= 'z') {
            return static_cast<int>(std::toupper(c));
        }
        if (c >= '0' && c <= '9') {
            return static_cast<int>(c);
        }
        if (c == '`') {
            return VK_OEM_3;
        }
    }
    if (t == "f1") return VK_F1;
    if (t == "f2") return VK_F2;
    if (t == "f3") return VK_F3;
    if (t == "f4") return VK_F4;
    if (t == "f5") return VK_F5;
    if (t == "f6") return VK_F6;
    if (t == "f7") return VK_F7;
    if (t == "f8") return VK_F8;
    if (t == "f9") return VK_F9;
    if (t == "f10") return VK_F10;
    if (t == "f11") return VK_F11;
    if (t == "f12") return VK_F12;
    if (t == "esc" || t == "escape") return VK_ESCAPE;
    if (t == "tab") return VK_TAB;
    if (t == "space") return VK_SPACE;
    if (t == "enter" || t == "return") return VK_RETURN;
    if (t == "left") return VK_LEFT;
    if (t == "right") return VK_RIGHT;
    if (t == "up") return VK_UP;
    if (t == "down") return VK_DOWN;
    if (t == "delete" || t == "del") return VK_DELETE;
    if (t == "home") return VK_HOME;
    if (t == "end") return VK_END;
    if (t == "backspace" || t == "bksp") return VK_BACK;
    if (t == "insert" || t == "ins") return VK_INSERT;
    if (t == "prior" || t == "pageup" || t == "pgup") return VK_PRIOR;
    if (t == "next" || t == "pagedown" || t == "pgdn") return VK_NEXT;
    return 0;
}

std::string VirtualKeyDisplay(int vk) {
    if (vk >= 'A' && vk <= 'Z') {
        return std::string(1, static_cast<char>(vk));
    }
    if (vk >= '0' && vk <= '9') {
        return std::string(1, static_cast<char>(vk));
    }
    switch (vk) {
    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";
    case VK_ESCAPE: return "Esc";
    case VK_TAB: return "Tab";
    case VK_SPACE: return "Space";
    case VK_RETURN: return "Enter";
    case VK_LEFT: return "Left";
    case VK_RIGHT: return "Right";
    case VK_UP: return "Up";
    case VK_DOWN: return "Down";
    case VK_DELETE: return "Del";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_BACK: return "Backspace";
    case VK_INSERT: return "Ins";
    case VK_PRIOR: return "PgUp";
    case VK_NEXT: return "PgDn";
    case VK_OEM_3: return "`";
    default: return {};
    }
}

} // namespace

bool KeyGesture::Matches(int vk, bool ctrl, bool shiftDown, bool altDown) const {
    if (IsEmpty()) {
        return false;
    }
    return virtualKey == vk && control == ctrl && shift == shiftDown && alt == altDown;
}

std::string KeyGesture::ToDisplayString() const {
    if (IsEmpty()) {
        return {};
    }
    std::string out;
    if (control) {
        out += "Ctrl+";
    }
    if (alt) {
        out += "Alt+";
    }
    if (shift) {
        out += "Shift+";
    }
    out += VirtualKeyDisplay(virtualKey);
    return out;
}

KeyGesture KeyGesture::Parse(const std::string& text) {
    KeyGesture g;
    if (text.empty()) {
        return g;
    }
    // Chords like "Ctrl+K Ctrl+O" are display-only.
    if (text.find(' ') != std::string::npos) {
        return g;
    }
    const auto parts = SplitPlus(text);
    if (parts.empty()) {
        return g;
    }
    for (const auto& part : parts) {
        const std::string t = ToLowerCopy(part);
        if (t == "ctrl" || t == "control" || t == "ctl") {
            g.control = true;
        } else if (t == "shift") {
            g.shift = true;
        } else if (t == "alt" || t == "menu") {
            g.alt = true;
        } else if (t == "win" || t == "windows" || t == "cmd") {
            // Not dispatched (no Win-key hook). Ignore so Parse still binds the rest.
        } else {
            g.virtualKey = VirtualKeyFromToken(part);
        }
    }
    if (g.virtualKey == 0) {
        return {};
    }
    return g;
}

Command::Command(ExecuteFn execute, CanExecuteFn canExecute)
    : m_execute(std::move(execute))
    , m_canExecute(std::move(canExecute)) {
}

bool Command::CanExecute() const {
    if (!m_execute) {
        return false;
    }
    return m_canExecute ? m_canExecute() : true;
}

void Command::Execute() {
    if (!CanExecute()) {
        return;
    }
    m_execute();
}

void Command::RaiseCanExecuteChanged() {
    m_canExecuteChanged.Invoke(this);
}

void CommandManager::Register(std::shared_ptr<Command> command) {
    if (!command) {
        return;
    }
    for (const auto& existing : m_commands) {
        if (existing.get() == command.get()) {
            return;
        }
    }
    m_commands.push_back(std::move(command));
}

void CommandManager::Unregister(Command* command) {
    if (!command) {
        return;
    }
    m_commands.erase(
        std::remove_if(m_commands.begin(), m_commands.end(),
            [command](const std::shared_ptr<Command>& c) { return c.get() == command; }),
        m_commands.end());
}

void CommandManager::Clear() {
    m_commands.clear();
}

std::shared_ptr<Command> CommandManager::Find(const std::string& id) const {
    if (id.empty()) {
        return nullptr;
    }
    for (const auto& cmd : m_commands) {
        if (cmd && cmd->GetId() == id) {
            return cmd;
        }
    }
    return nullptr;
}

bool CommandManager::TryExecute(int vk, bool ctrl, bool shift, bool alt) {
    for (const auto& cmd : m_commands) {
        if (!cmd || !cmd->GetGesture().Matches(vk, ctrl, shift, alt)) {
            continue;
        }
        if (cmd->CanExecute()) {
            cmd->Execute();
        }
        return true;
    }
    return false;
}

} // namespace CUI
