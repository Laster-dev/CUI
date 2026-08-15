import os
import re

def update_file(path, transformer):
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    new_content = transformer(content)
    if new_content != content:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Updated {path}")
        return True
    return False

# 1. UIElement.h and UIElement.cpp: add GetHWND()
def fix_ui_element():
    path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\UIElement.h'
    def trans_h(content):
        if '::HWND GetHWND() const;' not in content:
            content = content.replace('std::string GetId() const { return m_id; }', '::HWND GetHWND() const;\n    std::string GetId() const { return m_id; }')
        content = content.replace('ReadOnlyProperty<HWND> HWND;', 'ReadOnlyProperty<::HWND> HWND;')
        return content
    update_file(path_h, trans_h)

    path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\UIElement.cpp'
    def trans_cpp(content):
        if '::HWND UIElement::GetHWND() const' not in content:
            target = 'void UIElement::SetParent(UIElement* parent) {'
            replacement = '''::HWND UIElement::GetHWND() const {
    if (m_parent) return m_parent->GetHWND();
    return nullptr;
}

void UIElement::SetParent(UIElement* parent) {'''
            content = content.replace(target, replacement)
        return content
    update_file(path_cpp, trans_cpp)

# 2. WindowTitleBar.cpp
def fix_window_title_bar():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.cpp'
    def trans(content):
        content = re.sub(r'(?<=[(\s,])HWND(?=[\s*&)])', '::HWND', content)
        return content
    update_file(path, trans)

# 3. ContextMenu.cpp
def fix_context_menu():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ContextMenu.cpp'
    def trans(content):
        content = re.sub(r'(?<=[(\s,])HWND(?=[\s*&)])', '::HWND', content)
        return content
    update_file(path, trans)

# 4. TerminalControl.cpp
def fix_terminal_control():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TerminalControl.cpp'
    def trans(content):
        content = re.sub(r'(?<=[(\s,])HWND(?=[\s*&)])', '::HWND', content)
        return content
    update_file(path, trans)

# 5. DockManager.cpp
def fix_dock_manager():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\docking\DockManager.cpp'
    def trans(content):
        content = re.sub(r'(?<=[(\s,])HWND(?=[\s*&)])', '::HWND', content)
        return content
    update_file(path, trans)

fix_ui_element()
fix_window_title_bar()
fix_context_menu()
fix_terminal_control()
fix_dock_manager()
print("Fixed HWND references in all controls.")
