import os
import re

# 1. CheckBox.h
path_cb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CheckBox.h'
with open(path_cb, 'r', encoding='utf-8') as f:
    cb = f.read()

cb = re.sub(r'struct CheckBoxIsCheckedProperty \{[\s\S]*?\} IsChecked\{this\};\s*', '', cb)
cb_prop = '''    bool GetIsChecked() const { return m_state == CheckState::Checked; }
    void SetIsChecked(bool c) { SetState(c ? CheckState::Checked : CheckState::Unchecked); }

    struct CheckBoxIsCheckedProperty {
        CheckBox* owner;
        CheckBoxIsCheckedProperty& operator=(bool c) { owner->SetIsChecked(c); return *this; }
        operator bool() const { return owner->GetIsChecked(); }
        bool Get() const { return owner->GetIsChecked(); }
        bool operator()() const { return owner->GetIsChecked(); }
    } IsChecked{this};

'''
cb = cb.replace('    void SetState(CheckState state);', '    void SetState(CheckState state);\n' + cb_prop)
with open(path_cb, 'w', encoding='utf-8') as f:
    f.write(cb)

# 2. ToggleButton.h
path_tb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ToggleButton.h'
with open(path_tb, 'r', encoding='utf-8') as f:
    tb = f.read()

tb = re.sub(r'struct ToggleButtonIsCheckedProperty \{[\s\S]*?\} IsChecked\{this\};\s*', '', tb)
tb_prop = '''    struct ToggleButtonIsCheckedProperty {
        ToggleButton* owner;
        ToggleButtonIsCheckedProperty& operator=(bool c) { owner->SetChecked(c); return *this; }
        operator bool() const { return owner->GetIsChecked(); }
        bool Get() const { return owner->GetIsChecked(); }
        bool operator()() const { return owner->GetIsChecked(); }
    } IsChecked{this};

'''
tb = tb.replace('    void SetChecked(bool checked);', '    void SetChecked(bool checked);\n' + tb_prop)
with open(path_tb, 'w', encoding='utf-8') as f:
    f.write(tb)

# 3. WindowTitleBar.h & WindowTitleBar.cpp
path_wtb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.h'
with open(path_wtb, 'r', encoding='utf-8') as f:
    wtb = f.read()

wtb = wtb.replace('std::shared_ptr<MenuItem> AddMenu(const std::string& text);', 'std::shared_ptr<ContextMenu> AddMenu(const std::string& text);')
with open(path_wtb, 'w', encoding='utf-8') as f:
    f.write(wtb)

path_wtb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.cpp'
with open(path_wtb_cpp, 'r', encoding='utf-8') as f:
    wtb_cpp = f.read()

wtb_cpp = wtb_cpp.replace('std::shared_ptr<MenuItem> WindowTitleBar::WindowTitleBarMenuBarProperty::AddMenu',
                          'std::shared_ptr<ContextMenu> WindowTitleBar::WindowTitleBarMenuBarProperty::AddMenu')
with open(path_wtb_cpp, 'w', encoding='utf-8') as f:
    f.write(wtb_cpp)

print("Updated CheckBox, ToggleButton, WindowTitleBar.")
