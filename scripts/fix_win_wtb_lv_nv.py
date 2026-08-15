import os
import re

# 1. Window.h
path_win = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.h'
with open(path_win, 'r', encoding='utf-8') as f:
    win = f.read()

win = win.replace('HWND GetHWND() const { return m_hwnd; }', '::HWND GetHWND() const { return m_hwnd; }')
win = win.replace('Event<Window*, ThemeMode>& OnThemeChanged()', 'Event<Window*, CUI::ThemeMode>& OnThemeChanged()')
win = win.replace('Event<Window*, ThemeMode> m_onThemeChanged;', 'Event<Window*, CUI::ThemeMode> m_onThemeChanged;')
with open(path_win, 'w', encoding='utf-8') as f:
    f.write(win)

# 2. Window.cpp
path_win_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.cpp'
with open(path_win_cpp, 'r', encoding='utf-8') as f:
    wcpp = f.read()

wcpp = re.sub(r'void Window::SetThemeModeWithRipple\([^)]+\)', 'void Window::SetThemeModeWithRipple(CUI::ThemeMode theme, Point origin)', wcpp)
with open(path_win_cpp, 'w', encoding='utf-8') as f:
    f.write(wcpp)

# 3. WindowTitleBar.h
path_wtb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.h'
with open(path_wtb, 'r', encoding='utf-8') as f:
    wtb = f.read()

wtb = wtb.replace('std::shared_ptr<MenuBar> m_menuBar;', 'std::shared_ptr<CUI::MenuBar> m_menuBar;')
wtb = wtb.replace('MenuBar& GetMenuBar()', 'CUI::MenuBar& GetMenuBar()')
wtb = wtb.replace('const MenuBar& GetMenuBar() const', 'const CUI::MenuBar& GetMenuBar() const')
with open(path_wtb, 'w', encoding='utf-8') as f:
    f.write(wtb)

# 4. ListView.h
path_lv = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListView.h'
with open(path_lv, 'r', encoding='utf-8') as f:
    lv = f.read()
if 'struct ListViewCaretIndexProperty' not in lv:
    lv_prop = '''    struct ListViewCaretIndexProperty {
        ListView* owner = nullptr;
        ListViewCaretIndexProperty() = default;
        explicit ListViewCaretIndexProperty(ListView* o) : owner(o) {}
        ListViewCaretIndexProperty& operator=(int idx) { if (owner) owner->SetCaretIndex(idx); return *this; }
        operator int() const { return owner ? owner->GetCaretIndex() : 0; }
        int Get() const { return owner ? owner->GetCaretIndex() : 0; }
    } CaretIndex;

'''
    lv = lv.replace('    int GetCaretIndex() const { return m_caretRow; }', lv_prop + '    int GetCaretIndex() const { return m_caretRow; }')
    with open(path_lv, 'w', encoding='utf-8') as f:
        f.write(lv)

# 5. NavigationView.cpp
path_nv_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NavigationView.cpp'
with open(path_nv_cpp, 'r', encoding='utf-8') as f:
    nv_cpp = f.read()

nv_cpp = nv_cpp.replace('item->SelectsOnInvoked()', 'item->GetSelectsOnInvoked()')
with open(path_nv_cpp, 'w', encoding='utf-8') as f:
    f.write(nv_cpp)

print("Applied fixes to Window, WindowTitleBar, ListView, NavigationView.")
