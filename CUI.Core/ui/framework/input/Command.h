#pragma once
#include "../core/Event.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace CUI {

struct KeyGesture {
    int virtualKey = 0;
    bool control = false;
    bool shift = false;
    bool alt = false;

    bool IsEmpty() const { return virtualKey == 0; }
    bool Matches(int vk, bool ctrl, bool shiftDown, bool altDown) const;
    std::string ToDisplayString() const;
    static KeyGesture Parse(const std::string& text);
};

class Command {
public:
    using ExecuteFn = std::function<void()>;
    using CanExecuteFn = std::function<bool()>;

    Command() = default;
    explicit Command(ExecuteFn execute, CanExecuteFn canExecute = nullptr);

    void SetId(std::string id) { m_id = std::move(id); }
    const std::string& GetId() const { return m_id; }

    void SetLabel(std::string label) { m_label = std::move(label); }
    const std::string& GetLabel() const { return m_label; }

    void SetGesture(KeyGesture gesture) { m_gesture = gesture; }
    void SetGesture(const std::string& shortcut) { m_gesture = KeyGesture::Parse(shortcut); }
    const KeyGesture& GetGesture() const { return m_gesture; }

    bool CanExecute() const;
    void Execute();
    void RaiseCanExecuteChanged();
    Event<Command*>& CanExecuteChanged() { return m_canExecuteChanged; }

private:
    std::string m_id;
    std::string m_label;
    KeyGesture m_gesture;
    ExecuteFn m_execute;
    CanExecuteFn m_canExecute;
    Event<Command*> m_canExecuteChanged;
};

class CommandManager {
public:
    void Register(std::shared_ptr<Command> command);
    void Unregister(Command* command);
    void Clear();
    std::shared_ptr<Command> Find(const std::string& id) const;
    bool TryExecute(int vk, bool ctrl, bool shift, bool alt);

private:
    std::vector<std::shared_ptr<Command>> m_commands;
};

} // namespace CUI
