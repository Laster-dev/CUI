# CUI.Gallery —— 代码审查报告

- 审查日期：2026-09-19
- 工程路径：`CUI.Gallery/`
- 规模：82 源文件 / 11 头文件（98 文件）
- 方式：只读静态审查

---

## 1. 范围与架构

| 文件 | 职责 |
|---|---|
| `src/chrome/GalleryShell.cpp` | 外壳框架：导航路由、页面缓存（PageCache + LRU）、搜索 |
| `.../GalleryStatusBar.cpp` | 底部状态栏，含轮询式动画 tick |
| `.../CommandsPage.cpp` | 命令面板示例页，全局 Command 注册 |
| 其余页面 `.cpp` | 各控件/能力演示页 |

---

## 2. P0 — 生命周期与 UAF

| # | 问题 | 位置 | 后果 | 建议 |
|---|---|---|---|---|
| 1 | 导航闭包捕获 `shared_ptr` 形成循环引用 | `src/chrome/GalleryShell.cpp:168, 190` | Shell 与页面**互相持有，永不释放**，切换页面时内存持续增长 | 闭包捕获 `weak_ptr`，回调内 `lock()` 后判空 |
| 2 | 析构未调用 `CancelAnimationTicks` | `.../GalleryStatusBar.cpp:326` | 对象析构后仍被逐帧回调 → **悬挂回调 UAF** | RAII 封装 tick 注册/注销，析构自动注销 |
| 3 | 每 500ms 无条件 `RequestWake` | `.../GalleryStatusBar.cpp:326` | 空闲期空转唤醒，持续占用 CPU / 阻止休眠 | 仅在有活动任务时请求唤醒 |
| 4 | 全局 Command 表持有悬垂指针；`OnMouseDown` 每次重复注册 | `.../CommandsPage.cpp:144` | 重复注册 + 页面销毁后全局表指向已释放对象 → UAF | 注册改为构造期一次性；注销在析构；全局表存 `weak_ptr` 或 id |

---

## 3. P1 — 正确性

| 问题 | 位置 | 说明与建议 |
|---|---|---|
| PageCache LRU 键不一致 | `GalleryShell.cpp` | 淘汰判定使用的键与插入键不一致会淘汰错误页面。统一 `tag` 语义并加断言 |
| 页面缓存未设容量上限 | `GalleryShell.cpp` | 演示页多时内存无界增长。设 LRU 容量（如 8）并显式释放 |
| 页面构造/销毁未做耗时统计 | — | 建议加切换耗时日志，便于定位重页面 |

---

## 4. P1 — 性能

- 逐帧创建 DirectWrite 对象（`CreateTextFormat` / `CreateTextLayout`）→ 按属性元组缓存（同 CUI.Core 建议）。
- 页面切换时若无缓存命中，同步构建可能导致掉帧 → 考虑首帧骨架 + 异步构建。
- 搜索框输入逐字符触发全量过滤 → 加 debounce（如 120ms）。

---

## 5. P2 — 工程质量

- `CUI.Gallery.vcxproj` 需确认全部配置带 `/utf-8`（仓库中存在配置漂移）。
- 演示页之间存在重复的列表/卡片构建代码，建议抽取 `GalleryCard`、`GallerySection` 等共享组件。
- 页面注册建议改为表驱动（元数据 + 工厂函数），避免手工维护路由与清单两处。

---

## 6. 优先修复建议

**P0（立即）**
1. 导航闭包改 `weak_ptr`（#1）
2. 状态栏 tick 注册 RAII 化，析构注销（#2）
3. 按需 `RequestWake`（#3）
4. Command 注册去重 + `weak_ptr`（#4）

**P1（本迭代）**
5. PageCache 键统一 + 容量上限
6. DirectWrite 对象缓存
7. 搜索 debounce

**P2**
8. 抽取共享演示组件
9. 页面注册表驱动
