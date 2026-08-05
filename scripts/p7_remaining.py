#!/usr/bin/env python3
import re
from pathlib import Path
from collections import Counter
c=Counter()
files=Counter()
for base in [Path('CUI/ui'), Path('CUI/showcase')]:
  for p in base.rglob('*'):
    if p.suffix.lower() not in {'.cpp','.h','.hpp'}: continue
    t=p.read_text(encoding='utf-8', errors='replace')
    for m in re.finditer(r'SetProperty\(\s*"([^"]+)"', t):
      c[m.group(1)]+=1
      files[str(p)]+=1
print('BY NAME:')
for name,n in c.most_common(60):
  print(f'{n:4} {name}')
print('BY FILE:')
for f,n in files.most_common():
  print(f'{n:4} {f}')
print('TOTAL', sum(c.values()))
