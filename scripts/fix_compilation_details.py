import os

# 1. Expander.h
path_exp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Expander.h'
with open(path_exp, 'r', encoding='utf-8') as f:
    exp = f.read()
exp = exp.replace('ExpandDirection m_expandDirection = ExpandDirection::Down;', 'CUI::ExpandDirection m_expandDirection = CUI::ExpandDirection::Down;')
with open(path_exp, 'w', encoding='utf-8') as f:
    f.write(exp)

# 2. NumberBox.cpp
path_nb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NumberBox.cpp'
with open(path_nb_cpp, 'r', encoding='utf-8') as f:
    nb_cpp = f.read()
nb_cpp = nb_cpp.replace('FormatCUI::Value', 'FormatValue')
nb_cpp = nb_cpp.replace('NotifyFieldChanged(PropertyId::ControlValue, Value(clamped));', 'NotifyFieldChanged(PropertyId::ControlValue, CUI::Value(clamped));')
with open(path_nb_cpp, 'w', encoding='utf-8') as f:
    f.write(nb_cpp)

# 3. PasswordBox.cpp
path_pb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\PasswordBox.cpp'
with open(path_pb_cpp, 'r', encoding='utf-8') as f:
    pb_cpp = f.read()
pb_cpp = pb_cpp.replace('IsPasswordRevealed()', 'GetIsPasswordRevealed()')
with open(path_pb_cpp, 'w', encoding='utf-8') as f:
    f.write(pb_cpp)

# 4. TextBox.cpp
path_tb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBox.cpp'
with open(path_tb_cpp, 'r', encoding='utf-8') as f:
    tb_cpp = f.read()
tb_cpp = tb_cpp.replace('!IsPasswordRevealed()', '!m_isPasswordRevealed')
with open(path_tb_cpp, 'w', encoding='utf-8') as f:
    f.write(tb_cpp)

print("Updated four files.")
