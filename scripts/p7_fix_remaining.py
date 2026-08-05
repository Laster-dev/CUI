#!/usr/bin/env python3
from pathlib import Path
import re

vis_re = re.compile(
    r'SetProperty\(\s*"visibility"\s*,\s*Value\((.+)\)\s*\)'
)

def fix_vis(expr: str) -> str | None:
    expr = expr.strip()
    m = re.match(r'(.+?)\s*\?\s*"Visible"\s*:\s*"Collapsed"', expr)
    if m:
        return f'SetVisibility({m.group(1).strip()} ? Visibility::Visible : Visibility::Collapsed)'
    m = re.match(r'(.+?)\s*\?\s*"Collapsed"\s*:\s*"Visible"', expr)
    if m:
        return f'SetVisibility({m.group(1).strip()} ? Visibility::Collapsed : Visibility::Visible)'
    return None

files = [
  Path('CUI/ui/framework/controls/CollapsePanel.cpp'),
  Path('CUI/ui/framework/controls/NavigationView.cpp'),
  Path('CUI/ui/framework/controls/TerminalControl.cpp'),
]
for p in files:
    t = p.read_text(encoding='utf-8')
    def repl(m):
        r = fix_vis(m.group(1))
        if r is None:
            print('UNHANDLED', p, m.group(0))
            return m.group(0)
        return r
    nt = vis_re.sub(repl, t)
    if nt != t:
        p.write_text(nt, encoding='utf-8')
        print('fixed', p)

nv = Path('CUI/ui/framework/controls/NavigationView.cpp')
t = nv.read_text(encoding='utf-8')
t2 = re.sub(r'\n\s*SetProperty\("openPaneLength", Value\(m_openPaneLength\)\);', '', t)
t2 = re.sub(r'\n\s*SetProperty\("compactPaneLength", Value\(m_compactPaneLength\)\);', '', t2)
t2 = re.sub(r'\n\s*SetProperty\("header", Value\(header\)\);', '', t2)
t2 = re.sub(r'\n\s*SetProperty\("paneTitle", Value\(title\)\);', '', t2)
if t2 != t:
    nv.write_text(t2, encoding='utf-8')
    print('nav props cleaned')

bb = Path('CUI/ui/framework/controls/BreadcrumbBar.cpp')
t = bb.read_text(encoding='utf-8')
t2 = re.sub(
    r'\n\s*SetProperty\("activeColor", Value\(ThemeManager::Instance\(\)\.GetColor\("accentColor"\)\)\);',
    '', t)
if t2 != t:
    bb.write_text(t2, encoding='utf-8')
    print('breadcrumb cleaned')

for p, name in [
    (Path('CUI/ui/framework/controls/DatePicker.cpp'), 'dateStr'),
    (Path('CUI/ui/framework/controls/TimePicker.cpp'), 'timeStr'),
]:
    t = p.read_text(encoding='utf-8')
    t2 = re.sub(rf'\n\s*SetProperty\("{name}", Value\(GetFormatted\w+\(\)\)\);', '', t)
    if t2 != t:
        p.write_text(t2, encoding='utf-8')
        print('cleaned', p)

tc = Path('CUI/ui/framework/controls/ToastCenter.cpp')
t = tc.read_text(encoding='utf-8')
t2 = re.sub(r'\n\s*SetProperty\("isHitTestVisible", Value\(true\)\);', '', t)
if t2 != t:
    tc.write_text(t2, encoding='utf-8')
    print('toastcenter cleaned')

print('done')
