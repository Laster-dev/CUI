import os
import re

# 1. CheckBox.h
path_cb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CheckBox.h'
with open(path_cb, 'r', encoding='utf-8') as f:
    cb = f.read()

cb = cb.replace('bool IsThreeState() const { return m_isThreeState; }', 'bool GetIsThreeState() const { return m_isThreeState; }')
if 'struct CheckBoxIsThreeStateProperty' not in cb:
    prop = '''    struct CheckBoxIsThreeStateProperty {
        CheckBox* owner;
        CheckBoxIsThreeStateProperty& operator=(bool t) { owner->SetIsThreeState(t); return *this; }
        operator bool() const { return owner->GetIsThreeState(); }
        bool Get() const { return owner->GetIsThreeState(); }
    } IsThreeState{this};

'''
    cb = cb.replace('    Event<CheckBox*, CheckState>& OnCheckStateChanged()', prop + '    Event<CheckBox*, CheckState>& OnCheckStateChanged()')
    with open(path_cb, 'w', encoding='utf-8') as f:
        f.write(cb)

# 2. RatingControl.h
path_rc = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RatingControl.h'
with open(path_rc, 'r', encoding='utf-8') as f:
    rc = f.read()

rc = rc.replace('bool IsReadOnly() const { return m_isReadOnly; }', 'bool GetIsReadOnly() const { return m_isReadOnly; }')
if 'struct RatingControlIsReadOnlyProperty' not in rc:
    rc_prop = '''    struct RatingControlIsReadOnlyProperty {
        RatingControl* owner;
        RatingControlIsReadOnlyProperty& operator=(bool r) { owner->SetIsReadOnly(r); return *this; }
        operator bool() const { return owner->GetIsReadOnly(); }
        bool Get() const { return owner->GetIsReadOnly(); }
    } IsReadOnly{this};

'''
    rc = rc.replace('    bool IsClearEnabled() const', rc_prop + '    bool IsClearEnabled() const')
    with open(path_rc, 'w', encoding='utf-8') as f:
        f.write(rc)

# 3. CheckBoxPage.cpp
path_cb_page = r'E:\C++project\CUI\CUI.Gallery\src\pages\BasicInput\CheckBoxPage.cpp'
with open(path_cb_page, 'r', encoding='utf-8') as f:
    cbp = f.read()
cbp = cbp.replace('wifi->Checked->Bind(', 'wifi->Checked.Bind(')
cbp = cbp.replace('bluetooth->Checked->Bind(', 'bluetooth->Checked.Bind(')
cbp = cbp.replace('airplane->Checked->Bind(', 'airplane->Checked.Bind(')
with open(path_cb_page, 'w', encoding='utf-8') as f:
    f.write(cbp)

print("Applied last two fixes.")
