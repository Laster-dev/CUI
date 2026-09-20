"""一次性脚本：给 Widget.h / Widgets.h 的单行链式方法加 const，并统一 return self()。"""
import re
import pathlib

FILES = [
    r"E:\C++project\CUI\CUI.Core\ui\framework\core\Widget.h",
    r"E:\C++project\CUI\CUI.Core\ui\framework\core\Widgets.h",
]

# 单行链式方法：<Ret>& <Name>(<args>) { <body> return *this|self(); }
PAT = re.compile(
    r"^(?P<indent>\s*)"
    r"(?P<sig>[A-Za-z_][\w:<>, ]*&\s+\w+\s*\([^{;]*?\))"
    r"(?P<gap>\s*)"
    r"\{(?P<body>.*?)return (?:\*this|self\(\));(?P<tail>\s*)\}(?P<end>\s*)$"
)

for f in FILES:
    p = pathlib.Path(f)
    text = p.read_text(encoding="utf-8")
    out, changed = [], 0
    for line in text.splitlines(keepends=True):
        raw = line.rstrip("\r\n")
        m = PAT.match(raw)
        if not m:
            out.append(line)
            continue
        sig = m.group("sig")
        # 模板前缀（template<class V> ...）保持不动，只对签名加 const
        if not sig.rstrip().endswith("const"):
            sig = sig.rstrip() + " const"
        newline = (f"{m.group('indent')}{sig}{m.group('gap')}"
                   f"{{{m.group('body')}return self();{m.group('tail')}}}")
        out.append(newline + ("\n" if line.endswith("\n") else ""))
        changed += 1
    p.write_text("".join(out), encoding="utf-8")
    print(f"{p.name}: {changed} 处链式方法改为 const")
