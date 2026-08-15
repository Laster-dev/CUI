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

# 1. Update Expander.h
def fix_expander_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Expander.h'
    def trans(content):
        if 'struct ExpanderIsExpandedProperty' not in content:
            target = '    const std::string& GetHeader() const'
            replacement = '''    struct ExpanderIsExpandedProperty {
        Expander* owner;
        ExpanderIsExpandedProperty& operator=(bool exp) { owner->SetIsExpanded(exp); return *this; }
        operator bool() const { return owner->IsExpanded(); }
        bool Get() const { return owner->IsExpanded(); }
    } IsExpanded{this};

    struct ExpanderContentProperty {
        Expander* owner;
        ExpanderContentProperty& operator=(std::shared_ptr<UIElement> c) { owner->SetContent(std::move(c)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner->GetContent(); }
        std::shared_ptr<UIElement> Get() const { return owner->GetContent(); }
        std::shared_ptr<UIElement> operator->() const { return owner->GetContent(); }
    } Content{this};

    struct ExpanderExpandDirectionProperty {
        Expander* owner;
        ExpanderExpandDirectionProperty& operator=(ExpandDirection d) { owner->SetExpandDirection(d); return *this; }
        operator ExpandDirection() const { return owner->GetExpandDirection(); }
        ExpandDirection Get() const { return owner->GetExpandDirection(); }
    } ExpandDirection{this};

    const std::string& GetHeader() const'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

# 2. Update TextBox.h
def fix_textbox_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBox.h'
    def trans(content):
        if 'struct TextBoxIsReadOnlyProperty' not in content:
            target = '    virtual std::wstring GetDisplayedText() const;'
            replacement = '''    struct TextBoxIsReadOnlyProperty {
        TextBox* owner;
        TextBoxIsReadOnlyProperty& operator=(bool r) { owner->SetIsReadOnly(r); return *this; }
        operator bool() const { return owner->IsReadOnly(); }
        bool Get() const { return owner->IsReadOnly(); }
    } IsReadOnly{this};

    struct TextBoxIsPasswordModeProperty {
        TextBox* owner;
        TextBoxIsPasswordModeProperty& operator=(bool p) { owner->SetIsPasswordMode(p); return *this; }
        operator bool() const { return owner->IsPasswordMode(); }
        bool Get() const { return owner->IsPasswordMode(); }
    } IsPasswordMode{this};

    struct TextBoxShowRevealButtonProperty {
        TextBox* owner;
        TextBoxShowRevealButtonProperty& operator=(bool s) { owner->SetShowRevealButton(s); return *this; }
        operator bool() const { return owner->GetShowRevealButton(); }
        bool Get() const { return owner->GetShowRevealButton(); }
    } ShowRevealButton{this};

    struct TextBoxAcceptsReturnProperty {
        TextBox* owner;
        TextBoxAcceptsReturnProperty& operator=(bool a) { owner->SetAcceptsReturn(a); return *this; }
        operator bool() const { return owner->GetAcceptsReturn(); }
        bool Get() const { return owner->GetAcceptsReturn(); }
    } AcceptsReturn{this};

    struct TextBoxTextWrappingProperty {
        TextBox* owner;
        TextBoxTextWrappingProperty& operator=(bool w) { owner->SetTextWrapping(w); return *this; }
        operator bool() const { return owner->GetTextWrapping(); }
        bool Get() const { return owner->GetTextWrapping(); }
    } TextWrapping{this};

    struct TextBoxAllowDropProperty {
        TextBox* owner;
        TextBoxAllowDropProperty& operator=(bool d) { owner->SetAllowDrop(d); return *this; }
        operator bool() const { return owner->GetAllowDrop(); }
        bool Get() const { return owner->GetAllowDrop(); }
    } AllowDrop{this};

    virtual std::wstring GetDisplayedText() const;'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

fix_expander_h()
fix_textbox_h()
print("Fixed Expander and TextBox headers.")
