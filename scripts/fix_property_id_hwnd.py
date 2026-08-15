import os
import re
import sys

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

# 1. Update PropertyId.h & PropertyId.cpp
def update_property_id():
    path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\core\PropertyId.h'
    def trans_h(content):
        if 'IsCloseVisible' not in content:
            content = content.replace(
                '    Tag,\n    Count',
                '    Tag,\n    IsCloseVisible,\n    IsSettingsVisible,\n    Count'
            )
        return content
    update_file(path_h, trans_h)

    path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\core\PropertyId.cpp'
    def trans_cpp(content):
        if '"isCloseVisible"' not in content:
            content = content.replace(
                '"offsetX", "offsetY", "spacing", "corner", "id", "tag",',
                '"offsetX", "offsetY", "spacing", "corner", "id", "tag", "isCloseVisible", "isSettingsVisible",'
            )
        return content
    update_file(path_cpp, trans_cpp)

# 2. Fix HWND type qualifications in headers
def fix_hwnd_qualifications():
    files = [
        r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ContextMenu.h',
        r'E:\C++project\CUI\CUI.Core\ui\framework\controls\docking\DockFloatWindow.h',
        r'E:\C++project\CUI\CUI.Core\ui\framework\controls\docking\DockManager.h',
        r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TerminalControl.h',
    ]
    for p in files:
        def trans(content):
            content = re.sub(r'(?<=[(\s,])HWND(?=[\s*&)])', '::HWND', content)
            return content
        update_file(p, trans)

update_property_id()
fix_hwnd_qualifications()
print("Fixed PropertyId and HWND qualifications.")
