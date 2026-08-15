with open(r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CheckBox.h', 'r', encoding='utf-8') as f:
    cb = f.read()

cb = cb.replace('    Event<CheckBox*, CheckState>& OnCheckStateChanged()', '''    struct CheckBoxIsThreeStateProperty {
        CheckBox* owner;
        CheckBoxIsThreeStateProperty& operator=(bool t) { owner->SetIsThreeState(t); return *this; }
        operator bool() const { return owner->GetIsThreeState(); }
        bool Get() const { return owner->GetIsThreeState(); }
        bool operator()() const { return owner->GetIsThreeState(); }
    } IsThreeState{this};

    Event<CheckBox*, CheckState>& OnCheckStateChanged()''')

with open(r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CheckBox.h', 'w', encoding='utf-8') as f:
    f.write(cb)

print('Added IsThreeState property struct to CheckBox.h')
