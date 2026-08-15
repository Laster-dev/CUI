import os

# 1. Expander.h
path_exp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Expander.h'
with open(path_exp, 'r', encoding='utf-8') as f:
    exp = f.read()

exp = exp.replace('bool IsExpanded() const { return m_isExpanded; }', 'bool GetIsExpanded() const { return m_isExpanded; }')
target = '    void SetContent(std::shared_ptr<UIElement> content);'
replacement = '''    struct ExpanderContentProperty {
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

    void SetContent(std::shared_ptr<UIElement> content);'''
exp = exp.replace(target, replacement)

with open(path_exp, 'w', encoding='utf-8') as f:
    f.write(exp)

# 2. TextBox.h
path_tb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBox.h'
with open(path_tb, 'r', encoding='utf-8') as f:
    tb = f.read()

tb = tb.replace('bool IsPasswordMode() const { return m_isPasswordMode; }', 'bool GetIsPasswordMode() const { return m_isPasswordMode; }')
tb = tb.replace('bool IsPasswordRevealed() const { return m_isPasswordRevealed; }', 'bool GetIsPasswordRevealed() const { return m_isPasswordRevealed; }')
tb = tb.replace('bool IsReadOnly() const { return m_isReadOnly; }', 'bool GetIsReadOnly() const { return m_isReadOnly; }')

props = '''    struct TextBoxIsPasswordModeProperty {
        TextBox* owner;
        TextBoxIsPasswordModeProperty& operator=(bool p) { owner->SetIsPasswordMode(p); return *this; }
        operator bool() const { return owner->GetIsPasswordMode(); }
        bool Get() const { return owner->GetIsPasswordMode(); }
    } IsPasswordMode{this};

    struct TextBoxAllowDropProperty {
        TextBox* owner;
        TextBoxAllowDropProperty& operator=(bool d) { owner->SetAllowDrop(d); return *this; }
        operator bool() const { return owner->GetAllowDrop(); }
        bool Get() const { return owner->GetAllowDrop(); }
    } AllowDrop{this};

'''
tb = tb.replace('    void SetCompositionString(const std::wstring& compStr);', props + '    void SetCompositionString(const std::wstring& compStr);')

with open(path_tb, 'w', encoding='utf-8') as f:
    f.write(tb)

# 3. TextBox.cpp: replace IsReadOnly() -> m_isReadOnly and IsPasswordMode() -> m_isPasswordMode
path_tb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBox.cpp'
with open(path_tb_cpp, 'r', encoding='utf-8') as f:
    tb_cpp = f.read()

tb_cpp = tb_cpp.replace('IsReadOnly()', 'm_isReadOnly')
tb_cpp = tb_cpp.replace('IsPasswordMode()', 'm_isPasswordMode')

with open(path_tb_cpp, 'w', encoding='utf-8') as f:
    f.write(tb_cpp)

print("Applied minimal edits to Expander and TextBox.")
