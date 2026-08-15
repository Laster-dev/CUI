import os
import re

# 1. RatingControl.cpp
path_rc = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RatingControl.cpp'
with open(path_rc, 'r', encoding='utf-8') as f:
    rc = f.read()

rc = rc.replace('case PropertyId::ControlValue: return Value(m_value);', 'case PropertyId::ControlValue: return CUI::Value(m_value);')
rc = rc.replace('case PropertyId::Maximum: return Value(static_cast<float>(m_maxRating));', 'case PropertyId::Maximum: return CUI::Value(static_cast<float>(m_maxRating));')
rc = rc.replace('case PropertyId::Step: return Value(m_step);', 'case PropertyId::Step: return CUI::Value(m_step);')
rc = rc.replace('case PropertyId::IsReadOnly: return Value(m_isReadOnly);', 'case PropertyId::IsReadOnly: return CUI::Value(m_isReadOnly);')
rc = rc.replace('case PropertyId::IsClearEnabled: return Value(m_isClearEnabled);', 'case PropertyId::IsClearEnabled: return CUI::Value(m_isClearEnabled);')

rc = rc.replace('NotifyFieldChanged(PropertyId::ControlValue, Value(val));', 'NotifyFieldChanged(PropertyId::ControlValue, CUI::Value(val));')
rc = rc.replace('NotifyFieldChanged(PropertyId::Maximum, Value(static_cast<float>(maxRating)));', 'NotifyFieldChanged(PropertyId::Maximum, CUI::Value(static_cast<float>(maxRating)));')
rc = rc.replace('NotifyFieldChanged(PropertyId::Step, Value(step));', 'NotifyFieldChanged(PropertyId::Step, CUI::Value(step));')
rc = rc.replace('NotifyFieldChanged(PropertyId::IsReadOnly, Value(readOnly));', 'NotifyFieldChanged(PropertyId::IsReadOnly, CUI::Value(readOnly));')
rc = rc.replace('NotifyFieldChanged(PropertyId::IsClearEnabled, Value(enabled));', 'NotifyFieldChanged(PropertyId::IsClearEnabled, CUI::Value(enabled));')

with open(path_rc, 'w', encoding='utf-8') as f:
    f.write(rc)

# 2. ListBox.h
path_lb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListBox.h'
with open(path_lb, 'r', encoding='utf-8') as f:
    lb = f.read()

if 'void SetSelectedItem(const std::string& item)' not in lb:
    setter = '''    std::string GetSelectedItem() const;
    void SetSelectedItem(const std::string& item) {
        for (size_t i = 0; i < GetItemCount(); ++i) {
            if (GetItemAt(i) == item) {
                SetSelectedIndex(static_cast<int>(i));
                return;
            }
        }
        SetSelectedIndex(-1);
    }'''
    lb = lb.replace('    std::string GetSelectedItem() const;', setter)
    with open(path_lb, 'w', encoding='utf-8') as f:
        f.write(lb)

# 3. Window.h
path_win = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.h'
with open(path_win, 'r', encoding='utf-8') as f:
    win = f.read()

win = re.sub(r'Event<Window\*,\s*ThemeMode>&', 'Event<Window*, CUI::ThemeMode>&', win)
win = re.sub(r'Event<Window\*,\s*ThemeMode>\s+m_onThemeChanged;', 'Event<Window*, CUI::ThemeMode> m_onThemeChanged;', win)
with open(path_win, 'w', encoding='utf-8') as f:
    f.write(win)

# 4. Window.cpp
path_win_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.cpp'
with open(path_win_cpp, 'r', encoding='utf-8') as f:
    wcpp = f.read()

wcpp = re.sub(r'\bHWND\b\s+(other|fore|hwnd|wnd)\b', r'::HWND \1', wcpp)
wcpp = re.sub(r'\(HWND\)', r'(::HWND)', wcpp)
wcpp = re.sub(r'reinterpret_cast<HWND>', r'reinterpret_cast<::HWND>', wcpp)
wcpp = re.sub(r'static_cast<HWND>', r'static_cast<::HWND>', wcpp)

with open(path_win_cpp, 'w', encoding='utf-8') as f:
    f.write(wcpp)

print("Applied fixes to RatingControl.cpp, ListBox.h, Window.h, Window.cpp")
