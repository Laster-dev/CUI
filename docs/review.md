# CUI 代码审查总览（索引）

- 审查日期：2026-09-19
- 仓库：`e:/C++project/CUI/` —— CUI.Core 框架 + 9 个应用工程 + scripts / tools / docs
- 方式：只读静态审查（未修改任何源码）
- 分册：见 `docs/review/` 下各 `<项目>——review.md`

---

## 1. 分册索引

| 分册 | 主要风险 |
|---|---|
| [CUI.Core——review.md](review/CUI.Core——review.md) | 动画 UAF、拖拽 UAF、docking 越界与回调内 delete、终端注入面、`Event` O(n²)、Markdown DoS |
| [CUI——review.md](review/CUI——review.md) | 导航闭包循环引用、`g_streamImage` 数据竞争、GDI 句柄泄漏、LRU 键不一致 |
| [CUI.Gallery——review.md](review/CUI.Gallery——review.md) | 导航闭包循环引用、状态栏悬挂 tick、Command 重复注册与悬垂 |
| [AutoGuard——review.md](review/AutoGuard——review.md) | **误删目标文件**、伪造"已验证"、`schtasks` 注入、CSV 注入、COM 单元破坏 |
| [EverythingNEO——review.md](review/EverythingNEO——review.md) | **DB 名称偏移损坏**、USN 自 join 死锁、锁内写共享容器、USN 去重/级联缺失 |
| [RegeditPlus——review.md](review/RegeditPlus——review.md) | 可删根配置单元、重命名权限位错误、`FindNext` 卡 UI |
| [Patcher——review.md](review/Patcher——review.md) | **签名剥离条件写反截断文件**、PE 越界解析、盲补丁 |
| [Calc——review.md](review/Calc——review.md) | 构建配置漂移（`/utf-8`）、精度策略 |
| [Demo——review.md](review/Demo——review.md) | 构建配置漂移（`/utf-8`） |
| [scripts——review.md](review/scripts——review.md) | 缺 `newline=''` 污染 CRLF、硬编码路径、非幂等、无备份 |
| [tools——review.md](review/tools——review.md) | MCP 依赖的 `AutomationPipeServer` 代码 0 命中（功能不可用） |
| [Build——review.md](review/Build——review.md) | 全仓库构建配置漂移、缺 LICENSE、README 链接 404 |

---

## 2. 跨项目横切 TOP 问题

**A. 生命周期（出现于 Gallery / CUI / CUI.Core 三处）**
- 导航闭包捕获 `shared_ptr` → 循环引用（`GalleryShell.cpp:168,190`；`main.cpp:331,368`）
- 析构未注销回调 → 悬挂 UAF（`GalleryStatusBar.cpp:326`；`AnimationService.cpp:309`）
- `DragDropService::AbortIfParticipant` 零调用点（`DragDropService.cpp:129`）

**B. 外部输入缺少边界与信任校验（出现于 EverythingNEO / Patcher / 终端 / Markdown）**
- USN 记录、PE 结构、转义序列、Markdown 嵌套 —— 四类解析器均缺上限/版本/深度校验

**C. 危险写操作缺少备份与回滚（出现于 AutoGuard / Patcher / RegeditPlus）**
- 误删目标文件、签名剥离写反、可删根键 —— 三者均可造成不可逆损失

**D. 逐帧创建昂贵资源（全仓库）**
- DirectWrite `CreateTextFormat`、`Event::Invoke` 拷贝 handler vector、GDI 对象未还原

**E. 工程卫生（全仓库）**
- `/utf-8` 配置漂移、工具集混用、`OutDir` 写死、脚本硬编码且不幂等、缺 LICENSE

---

## 3. 修复顺序建议

1. **阶段一（立即，阻断发布）**：EverythingNEO 数据与死锁 → AutoGuard 误删 → Patcher 截断 → 生命周期三件套 → Markdown 深度上限
2. **阶段二（本迭代）**：终端安全面 → docking 崩溃与序列化 → 性能四项（Event / DirectWrite / TerminalBuffer / Layout）
3. **阶段三（架构与工程卫生）**：拆分超长单元 → 属性系统与目录命名统一 → 构建统一 → 脚本幂等化 → LICENSE/CI

> 每册末节的"优先修复建议"含 P0/P1/P2 分级与具体位置，可直接作为任务清单使用。
