import os
import re

# 1. TextBlock.h
path_tbk = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBlock.h'
with open(path_tbk, 'r', encoding='utf-8') as f:
    tbk = f.read()

tbk_props = '''    struct TextBlockTextAlignProperty {
        TextBlock* owner;
        TextBlockTextAlignProperty& operator=(TextAlignment a) { owner->SetTextAlign(a); return *this; }
        operator TextAlignment() const { return owner->GetTextAlign(); }
        TextAlignment Get() const { return owner->GetTextAlign(); }
    } TextAlign{this};

    struct TextBlockLineSpacingProperty {
        TextBlock* owner;
        TextBlockLineSpacingProperty& operator=(float s) { owner->SetLineSpacing(s); return *this; }
        operator float() const { return owner->GetLineSpacing(); }
        float Get() const { return owner->GetLineSpacing(); }
    } LineSpacing{this};

'''
if 'struct TextBlockTextAlignProperty' not in tbk:
    tbk = tbk.replace('    TextAlignment GetTextAlign() const { return m_textAlign; }', tbk_props + '    TextAlignment GetTextAlign() const { return m_textAlign; }')
    with open(path_tbk, 'w', encoding='utf-8') as f:
        f.write(tbk)

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
    rc = rc.replace('    void SetIsReadOnly(bool readOnly);', '    void SetIsReadOnly(bool readOnly);\n' + rc_prop)
    with open(path_rc, 'w', encoding='utf-8') as f:
        f.write(rc)

# 3. CheckBox.h
path_cb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CheckBox.h'
with open(path_cb, 'r', encoding='utf-8') as f:
    cb = f.read()

cb = cb.replace('bool IsThreeState() const { return m_isThreeState; }', 'bool GetIsThreeState() const { return m_isThreeState; }')
if 'struct CheckBoxIsThreeStateProperty' not in cb:
    cb_prop = '''    struct CheckBoxIsThreeStateProperty {
        CheckBox* owner;
        CheckBoxIsThreeStateProperty& operator=(bool t) { owner->SetIsThreeState(t); return *this; }
        operator bool() const { return owner->GetIsThreeState(); }
        bool Get() const { return owner->GetIsThreeState(); }
    } IsThreeState{this};

'''
    cb = cb.replace('    void SetIsThreeState(bool threeState);', '    void SetIsThreeState(bool threeState);\n' + cb_prop)
    if 'void SetIsThreeState' not in cb:
        cb = cb.replace('    void SetIsThreeState(bool threeState) {', '    void SetIsThreeState(bool threeState) {\n' + cb_prop)
    with open(path_cb, 'w', encoding='utf-8') as f:
        f.write(cb)

print("Updated TextBlock.h, RatingControl.h, CheckBox.h.")
