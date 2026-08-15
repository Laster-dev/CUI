import os
import re

# 1. RangeSlider.h
path_rs = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RangeSlider.h'
with open(path_rs, 'r', encoding='utf-8') as f:
    rs = f.read()

# remove duplicate LowerValue and UpperValue structs
rs = re.sub(r'struct RangeSliderLowerProperty \{[\s\S]*?\} LowerValue;\s*', '', rs)
rs = re.sub(r'struct RangeSliderUpperProperty \{[\s\S]*?\} UpperValue;\s*', '', rs)
with open(path_rs, 'w', encoding='utf-8') as f:
    f.write(rs)

path_rs_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RangeSlider.cpp'
with open(path_rs_cpp, 'r', encoding='utf-8') as f:
    rs_cpp = f.read()
rs_cpp = rs_cpp.replace('LowerValue(this), UpperValue(this)', '')
rs_cpp = rs_cpp.replace('Step(this),  {', 'Step(this) {')
rs_cpp = rs_cpp.replace('Step(this), {', 'Step(this) {')
with open(path_rs_cpp, 'w', encoding='utf-8') as f:
    f.write(rs_cpp)

# 2. RatingControl.h & RatingControl.cpp
path_rc = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RatingControl.h'
with open(path_rc, 'r', encoding='utf-8') as f:
    rc = f.read()
rc = rc.replace('void SetProperty(PropertyId id, const Value& val) override;', 'void SetProperty(PropertyId id, const CUI::Value& val) override;')
rc = rc.replace('NotifyFieldChanged(PropertyId::ControlValue, Value(m_value));', 'NotifyFieldChanged(PropertyId::ControlValue, CUI::Value(m_value));')
rc = rc.replace('NotifyFieldChanged(PropertyId::Maximum, Value(static_cast<float>(m_maxRating)));', 'NotifyFieldChanged(PropertyId::Maximum, CUI::Value(static_cast<float>(m_maxRating)));')
rc = rc.replace('NotifyFieldChanged(PropertyId::Step, Value(m_step));', 'NotifyFieldChanged(PropertyId::Step, CUI::Value(m_step));')
rc = rc.replace('NotifyFieldChanged(PropertyId::IsReadOnly, Value(m_isReadOnly));', 'NotifyFieldChanged(PropertyId::IsReadOnly, CUI::Value(m_isReadOnly));')
rc = rc.replace('NotifyFieldChanged(PropertyId::IsClearEnabled, Value(m_isClearEnabled));', 'NotifyFieldChanged(PropertyId::IsClearEnabled, CUI::Value(m_isClearEnabled));')
with open(path_rc, 'w', encoding='utf-8') as f:
    f.write(rc)

path_rc_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RatingControl.cpp'
with open(path_rc_cpp, 'r', encoding='utf-8') as f:
    rc_cpp = f.read()
rc_cpp = rc_cpp.replace('void RatingControl::SetProperty(PropertyId id, const Value& val)', 'void RatingControl::SetProperty(PropertyId id, const CUI::Value& val)')
rc_cpp = rc_cpp.replace('NotifyFieldChanged(PropertyId::ControlValue, Value(v));', 'NotifyFieldChanged(PropertyId::ControlValue, CUI::Value(v));')
rc_cpp = rc_cpp.replace('NotifyFieldChanged(PropertyId::Maximum, Value(static_cast<float>(m_maxRating)));', 'NotifyFieldChanged(PropertyId::Maximum, CUI::Value(static_cast<float>(m_maxRating)));')
rc_cpp = rc_cpp.replace('NotifyFieldChanged(PropertyId::Step, Value(m_step));', 'NotifyFieldChanged(PropertyId::Step, CUI::Value(m_step));')
rc_cpp = rc_cpp.replace('NotifyFieldChanged(PropertyId::IsReadOnly, Value(m_isReadOnly));', 'NotifyFieldChanged(PropertyId::IsReadOnly, CUI::Value(m_isReadOnly));')
rc_cpp = rc_cpp.replace('NotifyFieldChanged(PropertyId::IsClearEnabled, Value(m_isClearEnabled));', 'NotifyFieldChanged(PropertyId::IsClearEnabled, CUI::Value(m_isClearEnabled));')
with open(path_rc_cpp, 'w', encoding='utf-8') as f:
    f.write(rc_cpp)

# 3. ListBox.h & ListBox.cpp
path_lb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListBox.h'
with open(path_lb, 'r', encoding='utf-8') as f:
    lb = f.read()

if 'struct ListBoxSelectedIndicesProperty' not in lb:
    lb_props = '''    struct ListBoxSelectedIndicesProperty {
        ListBox* owner;
        operator const std::unordered_set<int>&() const { return owner->GetSelectedIndices(); }
        const std::unordered_set<int>& Get() const { return owner->GetSelectedIndices(); }
        size_t size() const { return owner->GetSelectedIndices().size(); }
    } SelectedIndices{this};

    struct ListBoxSelectedItemProperty {
        ListBox* owner;
        ListBoxSelectedItemProperty& operator=(const std::string& item) { owner->SetSelectedItem(item); return *this; }
        operator std::string() const { return owner->GetSelectedItem(); }
        std::string Get() const { return owner->GetSelectedItem(); }
    } SelectedItem{this};

    struct ListBoxAllowDragProperty {
        ListBox* owner;
        ListBoxAllowDragProperty& operator=(bool d) { owner->SetAllowDrag(d); return *this; }
        operator bool() const { return owner->GetAllowDrag(); }
        bool Get() const { return owner->GetAllowDrag(); }
    } AllowDrag{this};

    struct ListBoxAllowDropProperty {
        ListBox* owner;
        ListBoxAllowDropProperty& operator=(bool d) { owner->SetAllowDrop(d); return *this; }
        operator bool() const { return owner->GetAllowDrop(); }
        bool Get() const { return owner->GetAllowDrop(); }
    } AllowDrop{this};

'''
    lb = lb.replace('    std::string GetSelectedItem() const;', lb_props + '    std::string GetSelectedItem() const;')
    with open(path_lb, 'w', encoding='utf-8') as f:
        f.write(lb)

path_lb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListBox.cpp'
with open(path_lb_cpp, 'r', encoding='utf-8') as f:
    lb_cpp = f.read()
lb_cpp = lb_cpp.replace('ListBox::ListBox() : SelectedIndices(this), SelectedItem(this), AllowDrag(this), AllowDrop(this) {', 'ListBox::ListBox() {')
with open(path_lb_cpp, 'w', encoding='utf-8') as f:
    f.write(lb_cpp)

# 4. ListView.h & ListView.cpp
path_lv = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListView.h'
with open(path_lv, 'r', encoding='utf-8') as f:
    lv = f.read()

if 'struct ListViewSelectedIndicesProperty' not in lv:
    lv_props = '''    struct ListViewSelectedIndicesProperty {
        ListView* owner;
        operator const std::unordered_set<int>&() const { return owner->GetSelectedIndices(); }
        const std::unordered_set<int>& Get() const { return owner->GetSelectedIndices(); }
        size_t size() const { return owner->GetSelectedIndices().size(); }
    } SelectedIndices{this};

    struct ListViewCaretIndexProperty {
        ListView* owner;
        ListViewCaretIndexProperty& operator=(int idx) { owner->SetCaretIndex(idx); return *this; }
        operator int() const { return owner->GetCaretIndex(); }
        int Get() const { return owner->GetCaretIndex(); }
    } CaretIndex{this};

'''
    lv = lv.replace('    int GetCaretIndex() const { return m_caretRow; }', lv_props + '    int GetCaretIndex() const { return m_caretRow; }')
    with open(path_lv, 'w', encoding='utf-8') as f:
        f.write(lv)

path_lv_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListView.cpp'
with open(path_lv_cpp, 'r', encoding='utf-8') as f:
    lv_cpp = f.read()
lv_cpp = lv_cpp.replace('ListView::ListView() : SelectedIndices(this), CaretIndex(this) {', 'ListView::ListView() {')
with open(path_lv_cpp, 'w', encoding='utf-8') as f:
    f.write(lv_cpp)

# 5. Window.h & Window.cpp
path_win = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.h'
with open(path_win, 'r', encoding='utf-8') as f:
    win = f.read()
win = re.sub(r'Event<Window\*,\s*ThemeMode>& OnThemeChanged\(\)', 'Event<Window*, CUI::ThemeMode>& OnThemeChanged()', win)
with open(path_win, 'w', encoding='utf-8') as f:
    f.write(win)

path_win_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.cpp'
with open(path_win_cpp, 'r', encoding='utf-8') as f:
    wcpp = f.read()
wcpp = wcpp.replace('originPoint', 'origin')
with open(path_win_cpp, 'w', encoding='utf-8') as f:
    f.write(wcpp)

# 6. WindowTitleBar.cpp
path_wtb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.cpp'
with open(path_wtb_cpp, 'r', encoding='utf-8') as f:
    wtb_cpp = f.read()
wtb_cpp = wtb_cpp.replace('m_menuBar = std::make_shared<MenuBar>();', 'm_menuBar = std::make_shared<CUI::MenuBar>();')
with open(path_wtb_cpp, 'w', encoding='utf-8') as f:
    f.write(wtb_cpp)

print("Applied clean final fixes.")
