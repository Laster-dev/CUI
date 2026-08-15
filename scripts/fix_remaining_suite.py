import os
import re

# 1. CheckBox.h
path_cb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CheckBox.h'
with open(path_cb, 'r', encoding='utf-8') as f:
    cb = f.read()

cb = cb.replace('bool IsChecked() const { return m_state == CheckState::Checked; }', 'bool GetIsChecked() const { return m_state == CheckState::Checked; }')
if 'struct CheckBoxIsCheckedProperty' not in cb:
    cb_prop = '''    struct CheckBoxIsCheckedProperty {
        CheckBox* owner;
        CheckBoxIsCheckedProperty& operator=(bool c) { owner->SetChecked(c); return *this; }
        operator bool() const { return owner->GetIsChecked(); }
        bool Get() const { return owner->GetIsChecked(); }
    } IsChecked{this};

'''
    cb = cb.replace('    void SetState(CheckState state);', '    void SetState(CheckState state);\n' + cb_prop)
    with open(path_cb, 'w', encoding='utf-8') as f:
        f.write(cb)

# 2. ToggleButton.h
path_tb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ToggleButton.h'
with open(path_tb, 'r', encoding='utf-8') as f:
    tb = f.read()

tb = tb.replace('bool IsChecked() const { return m_isChecked; }', 'bool GetIsChecked() const { return m_isChecked; }')
if 'struct ToggleButtonIsCheckedProperty' not in tb:
    tb_prop = '''    struct ToggleButtonIsCheckedProperty {
        ToggleButton* owner;
        ToggleButtonIsCheckedProperty& operator=(bool c) { owner->SetChecked(c); return *this; }
        operator bool() const { return owner->GetIsChecked(); }
        bool Get() const { return owner->GetIsChecked(); }
    } IsChecked{this};

'''
    tb = tb.replace('    void SetChecked(bool checked);', '    void SetChecked(bool checked);\n' + tb_prop)
    with open(path_tb, 'w', encoding='utf-8') as f:
        f.write(tb)

# 3. Slider.h
path_sl = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Slider.h'
with open(path_sl, 'r', encoding='utf-8') as f:
    sl = f.read()

sl = sl.replace('NotifyFieldChanged(PropertyId::Minimum, Value(minVal));', 'NotifyFieldChanged(PropertyId::Minimum, CUI::Value(minVal));')
sl = sl.replace('NotifyFieldChanged(PropertyId::Maximum, Value(maxVal));', 'NotifyFieldChanged(PropertyId::Maximum, CUI::Value(maxVal));')
sl = sl.replace('NotifyFieldChanged(PropertyId::Step, Value(s));', 'NotifyFieldChanged(PropertyId::Step, CUI::Value(s));')

if 'struct SliderMinimumProperty' not in sl:
    sl_props = '''    struct SliderMinimumProperty {
        Slider* owner;
        SliderMinimumProperty& operator=(float m) { owner->SetMinimum(m); return *this; }
        operator float() const { return owner->GetMinimum(); }
        float Get() const { return owner->GetMinimum(); }
    } Minimum{this};

    struct SliderMaximumProperty {
        Slider* owner;
        SliderMaximumProperty& operator=(float m) { owner->SetMaximum(m); return *this; }
        operator float() const { return owner->GetMaximum(); }
        float Get() const { return owner->GetMaximum(); }
    } Maximum{this};

    struct SliderStepProperty {
        Slider* owner;
        SliderStepProperty& operator=(float s) { owner->SetStep(s); return *this; }
        operator float() const { return owner->GetStep(); }
        float Get() const { return owner->GetStep(); }
    } Step{this};

'''
    sl = sl.replace('    float GetValue() const { return m_value; }', sl_props + '    float GetValue() const { return m_value; }')
    with open(path_sl, 'w', encoding='utf-8') as f:
        f.write(sl)

# 4. MarkdownView.h
path_md = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\MarkdownView.h'
with open(path_md, 'r', encoding='utf-8') as f:
    md = f.read()

if 'struct MarkdownViewMarkdownProperty' not in md:
    md_prop = '''    struct MarkdownViewMarkdownProperty {
        MarkdownView* owner;
        MarkdownViewMarkdownProperty& operator=(const std::string& md) { owner->SetMarkdown(md); return *this; }
        operator const std::string&() const { return owner->GetMarkdown(); }
        const std::string& Get() const { return owner->GetMarkdown(); }
    } Markdown{this};

'''
    md = md.replace('    void SetMarkdown(const std::string& markdown);', md_prop + '    void SetMarkdown(const std::string& markdown);')
    with open(path_md, 'w', encoding='utf-8') as f:
        f.write(md)

# 5. NavigationViewItem.h
path_nvi = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NavigationViewItem.h'
with open(path_nvi, 'r', encoding='utf-8') as f:
    nvi = f.read()

if 'struct NavItemContentProperty' not in nvi:
    nvi_prop = '''    struct NavItemContentProperty {
        NavigationViewItem* owner;
        NavItemContentProperty& operator=(const std::string& c) { owner->SetContent(c); return *this; }
        operator const std::string&() const { return owner->GetContent(); }
        const std::string& Get() const { return owner->GetContent(); }
    } Content{this};

'''
    nvi = nvi.replace('    void SetContent(const std::string& content);', nvi_prop + '    void SetContent(const std::string& content);')
    with open(path_nvi, 'w', encoding='utf-8') as f:
        f.write(nvi)

# 6. NavigationView.h & NavigationView.cpp
path_nv = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NavigationView.h'
with open(path_nv, 'r', encoding='utf-8') as f:
    nv = f.read()

nv = nv.replace('bool IsPaneOpen() const { return m_isPaneOpen; }', 'bool GetIsPaneOpen() const { return m_isPaneOpen; }')
nv = nv.replace('bool IsSettingsVisible() const { return m_settingsVisible; }', 'bool GetIsSettingsVisible() const { return m_settingsVisible; }')

if 'struct NavIsPaneOpenProperty' not in nv:
    nv_props = '''    struct NavIsPaneOpenProperty {
        NavigationView* owner;
        NavIsPaneOpenProperty& operator=(bool o) { owner->SetIsPaneOpen(o); return *this; }
        operator bool() const { return owner->GetIsPaneOpen(); }
        bool Get() const { return owner->GetIsPaneOpen(); }
    } IsPaneOpen{this};

    struct NavIsSettingsVisibleProperty {
        NavigationView* owner;
        NavIsSettingsVisibleProperty& operator=(bool v) { owner->SetIsSettingsVisible(v); return *this; }
        operator bool() const { return owner->GetIsSettingsVisible(); }
        bool Get() const { return owner->GetIsSettingsVisible(); }
    } IsSettingsVisible{this};

'''
    nv = nv.replace('    void SetPaneDisplayMode(NavigationViewPaneDisplayMode mode);', nv_props + '    void SetPaneDisplayMode(NavigationViewPaneDisplayMode mode);')
    with open(path_nv, 'w', encoding='utf-8') as f:
        f.write(nv)

# 7. WindowTitleBar.h
path_wtb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.h'
with open(path_wtb, 'r', encoding='utf-8') as f:
    wtb = f.read()

wtb_menu_prop = '''    struct WindowTitleBarMenuBarProperty {
        WindowTitleBar* owner = nullptr;
        WindowTitleBarMenuBarProperty() = default;
        explicit WindowTitleBarMenuBarProperty(WindowTitleBar* o) : owner(o) {}
        operator CUI::MenuBar&() const { return owner->GetMenuBar(); }
        CUI::MenuBar* operator->() const { return &owner->GetMenuBar(); }
        CUI::MenuBar& Get() const { return owner->GetMenuBar(); }
        std::shared_ptr<MenuItem> AddMenu(const std::string& text);
    } MenuBar;'''

wtb = re.sub(r'struct WindowTitleBarMenuBarProperty \{[\s\S]*?\} MenuBar;', wtb_menu_prop, wtb)
with open(path_wtb, 'w', encoding='utf-8') as f:
    f.write(wtb)

path_wtb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.cpp'
with open(path_wtb_cpp, 'r', encoding='utf-8') as f:
    wtb_cpp = f.read()
if 'WindowTitleBar::WindowTitleBarMenuBarProperty::AddMenu' not in wtb_cpp:
    wtb_cpp += '''

namespace CUI {
std::shared_ptr<MenuItem> WindowTitleBar::WindowTitleBarMenuBarProperty::AddMenu(const std::string& text) {
    return owner->GetMenuBar().AddMenu(text);
}
}
'''
    with open(path_wtb_cpp, 'w', encoding='utf-8') as f:
        f.write(wtb_cpp)

# 8. TextBlock.h
path_textblock = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBlock.h'
with open(path_textblock, 'r', encoding='utf-8') as f:
    tbk = f.read()

if 'struct TextBlockTextAlignProperty' not in tbk:
    tbk_props = '''    struct TextBlockTextAlignProperty {
        TextBlock* owner;
        TextBlockTextAlignProperty& operator=(TextAlign a) { owner->SetTextAlign(a); return *this; }
        operator TextAlign() const { return owner->GetTextAlign(); }
        TextAlign Get() const { return owner->GetTextAlign(); }
    } TextAlign{this};

    struct TextBlockLineSpacingProperty {
        TextBlock* owner;
        TextBlockLineSpacingProperty& operator=(float s) { owner->SetLineSpacing(s); return *this; }
        operator float() const { return owner->GetLineSpacing(); }
        float Get() const { return owner->GetLineSpacing(); }
    } LineSpacing{this};

'''
    tbk = tbk.replace('    TextAlign GetTextAlign() const { return m_textAlign; }', tbk_props + '    TextAlign GetTextAlign() const { return m_textAlign; }')
    with open(path_textblock, 'w', encoding='utf-8') as f:
        f.write(tbk)

print("Applied fixes to CheckBox, ToggleButton, Slider, MarkdownView, NavigationView, NavigationViewItem, WindowTitleBar, TextBlock.")
