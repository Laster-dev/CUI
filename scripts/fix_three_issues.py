import os
import re

# 1. NumberBox.h
path_nb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NumberBox.h'
with open(path_nb, 'r', encoding='utf-8') as f:
    nb = f.read()

nb = nb.replace('NotifyFieldChanged(PropertyId::Step, Value(s));', 'NotifyFieldChanged(PropertyId::Step, CUI::Value(s));')
nb = nb.replace('NotifyFieldChanged(PropertyId::Minimum, Value(minVal));', 'NotifyFieldChanged(PropertyId::Minimum, CUI::Value(minVal));')
nb = nb.replace('NotifyFieldChanged(PropertyId::Maximum, Value(maxVal));', 'NotifyFieldChanged(PropertyId::Maximum, CUI::Value(maxVal));')
with open(path_nb, 'w', encoding='utf-8') as f:
    f.write(nb)

# 2. NumberBox.cpp
path_nb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NumberBox.cpp'
with open(path_nb_cpp, 'r', encoding='utf-8') as f:
    nb_cpp = f.read()

nb_cpp = nb_cpp.replace('Value(m_value)', 'CUI::Value(m_value)')
nb_cpp = nb_cpp.replace('Value(m_minimum)', 'CUI::Value(m_minimum)')
nb_cpp = nb_cpp.replace('Value(m_maximum)', 'CUI::Value(m_maximum)')
nb_cpp = nb_cpp.replace('Value(m_step)', 'CUI::Value(m_step)')
nb_cpp = nb_cpp.replace('void NumberBox::SetProperty(PropertyId id, const Value& val)', 'void NumberBox::SetProperty(PropertyId id, const CUI::Value& val)')
nb_cpp = nb_cpp.replace('NotifyFieldChanged(PropertyId::ControlValue, Value(val));', 'NotifyFieldChanged(PropertyId::ControlValue, CUI::Value(val));')
with open(path_nb_cpp, 'w', encoding='utf-8') as f:
    f.write(nb_cpp)

# 3. Expander.cpp
path_exp_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Expander.cpp'
with open(path_exp_cpp, 'r', encoding='utf-8') as f:
    exp_cpp = f.read()

exp_cpp = exp_cpp.replace('void Expander::SetExpandDirection(ExpandDirection direction)', 'void Expander::SetExpandDirection(CUI::ExpandDirection direction)')
with open(path_exp_cpp, 'w', encoding='utf-8') as f:
    f.write(exp_cpp)

# 4. TextBox.cpp
path_tb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBox.cpp'
with open(path_tb_cpp, 'r', encoding='utf-8') as f:
    tb_cpp = f.read()

tb_cpp = tb_cpp.replace('GetShowRevealButton()', 'm_showRevealButton')
with open(path_tb_cpp, 'w', encoding='utf-8') as f:
    f.write(tb_cpp)

print("Fixed NumberBox, Expander.cpp, and TextBox.cpp.")
