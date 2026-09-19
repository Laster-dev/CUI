# CUI.Core —— 代码审查报告

- 审查日期：2026-09-19
- 工程路径：`CUI.Core/`
- 规模：140 头文件 / 118 实现文件（约 261 文件）
- 方式：只读静态审查
- **修复进度：见文末「修复状态」**

---

## 1. 范围与架构

CUI.Core 是自绘 UI 框架层，主要子系统：

| 子系统 | 目录 | 职责 |
|---|---|---|
| DSL | `framework/core/CUIDsl.h` | Fluent Builder 声明式构建 UI |
| 属性系统 | `core/Property*.h`、`core/BindableProperty.h` | 强类型 `Property<T>` / 弱类型 `PropertyId`+`Value` / `BindableProperty`+`CallbackProperty` 三套并存 |
| 布局引擎 | `core/Layout.cpp` | Flex / Grid / Canvas / Wrap / Dock / UniformGrid |
| 动画 | `core/AnimationService.cpp`、`AnimationManager`、`FrameScheduler` | 动画调度与逐帧驱动 |
| 主题 | `ThemeManager` | 设计 Token |
| 终端 | `controls/terminal/` | ConPTY + 转义序列状态机 + 屏幕/滚动缓冲 + DWrite 渲染 + UTF-8/16 编解码 |
| 停靠 | `controls/docking/` | DockManager / DockFloatWindow / 布局序列化 |
| 文本 | `text/markdown/` | Markdown 解析与排版 |
| 输入/拖拽 | `input/`、`dnd/` | 输入路由、DragDropService |
| 网络 | `network/` | 基础网络封装 |

---

## 2. P0 — 崩溃 / UAF

| # | 问题 | 位置 | 后果 | 建议 |
|---|---|---|---|---|
| 1 | `Tick` 用 `std::move` 覆盖动画列表，并持有裸指针 | `framework/core/AnimationService.cpp:309` | 本帧新增动画丢失 + 已完成对象 UAF | 拷贝到局部容器再遍历；句柄改为 id / `weak_ptr` |
| 2 | `AbortIfParticipant` **零调用点** | `framework/dnd/DragDropService.cpp:129` | 拖拽进行中销毁参与元素 → UAF | 接入元素析构路径与容器 `RemoveChild` 路径 |
| 3 | `BindableProperty` 持有 `Object&` | `framework/core/BindableProperty.h` | 外部对象先析构即悬垂引用 | 改 `weak_ptr` 或显式 `Unbind()` 契约 |
| 4 | `m_updating` 手写布尔标记 | `framework/core/BindableProperty.h` | 重入或异常后标记不复位，绑定永久失效 | RAII `UpdateGuard` |
| 5 | Pin 索引越界 | `framework/controls/docking/DockManager.cpp:1271-1273` | 越界读写崩溃 | 索引边界校验 + `at()` |
| 6 | 悬浮窗在自身回调中 `delete` | `DockManager.cpp:929` + `DockFloatWindow.cpp:732` | 回调返回后访问已释放对象（双重释放） | 改为 `PostMessage` 延迟销毁 / `DestroyWindow` |
| 7 | 布局反序列化丢失悬浮窗与未认领 pane，且不重置 autoHide | `framework/controls/docking/DockLayoutSerializer.cpp` | 用户布局静默丢失，状态残留 | 保留未认领 pane；Load 前重置 autoHide |

---

## 3. P0 — 安全（终端子系统）

| # | 问题 | 位置 | 后果 | 建议 |
|---|---|---|---|---|
| 8 | `m_commandLine` 无引号拼接 + `CreateProcessW(nullptr, ...)` | `controls/terminal/ConPtyBackend.cpp` | 可执行文件路径劫持 / 参数注入 | 显式传 `lpApplicationName`；参数逐项加引号转义 |
| 9 | OSC 52 静默读写系统剪贴板 | `controls/terminal/InputHandler.cpp` | 恶意输出窃取剪贴板 | 写需确认、读默认禁用 |
| 10 | OSC 8 超链接未校验协议 | `controls/terminal/InputHandler.cpp` | 诱导点击 `file://` / 其它协议 | 协议白名单 `http/https` |
| 11 | OSC/DCS 字符串无长度上限 | `controls/terminal/EscapeSequenceParser.cpp` | 内存耗尽 DoS | 设上限并降级丢弃 |
| 12 | UTF-8 解码无合法性校验 | `EscapeSequenceParser.cpp` | 超长序列 / 代理对 / 截断序列导致异常渲染 | 严格 UTF-8 校验 |
| 13 | `SosPmApc` 状态吞字节 | `EscapeSequenceParser.cpp` | 状态机卡死、后续输出错乱 | 明确状态转移与超时退出 |
| 14 | 写队列无上限；`stop()` 未终止子进程 | `ConPtyBackend.cpp` | 内存增长 + 残留进程 | 队列上限 + 背压；`stop()` 中 `TerminateProcess` 兜底 |

---

## 4. P1 — 性能

| 问题 | 位置 | 说明与建议 |
|---|---|---|
| `Invoke` 每次拷贝 handler vector | `framework/core/Event.h:38` | 事件密集场景 O(n²)。改为 `shared_ptr<const vector>` 快照，写时复制 |
| 逐原子调用 `CreateTextFormat` | `framework/text/markdown/MarkdownLayout.cpp` | 性能灾难。按 (family,size,weight,style) 缓存，配合 LRU |
| `Reflow` 约 24MB 临时拷贝 | `controls/terminal/TerminalBuffer.cpp` | 改为环形缓冲 + 增量重排 |
| 滚动 O(n·m) 行搬移 | `TerminalBuffer.cpp` | 环形缓冲 + 只搬移视口内行 |
| Grid Auto 列未清零 | `framework/core/Layout.cpp:430` (`ArrangeGrid`) | 上一轮测量结果粘滞。每轮初始化 |
| 布局全流程无 `isfinite` 防护 | `framework/core/Layout.cpp` 全文件 | NaN/Inf 沿树传播。输入与输出各加一次校验 |
| `Spring` 缓动 `t=1` 时返回值 ≠ 1 | `AnimationService.cpp` | 终值偏差。修正归一化 |

---

## 5. P1 — 架构与可读性

- **三套属性系统并存**：强类型 `Property<T>` / 弱类型 `PropertyId`+`Value` / `BindableProperty`+`CallbackProperty`。语义重叠、转换成本高。**建议**：明确"强类型为唯一真源、弱类型仅作样式表桥接"，并在《统一属性与链式页面API规范.md》中固化边界。
- **命名不符**：`framework/parser/StyleManager.cpp` 位于 `parser` 目录，实为硬编码样式表；且内含 `vscode-*` 产品类名——框架层不应耦合具体产品皮肤。**建议**：目录改名 `style/`，皮肤外置为资源文件。
- **超长单元**：
  - `framework/core/CUIDsl.h` 1604 行，其中 `ElementBuilder` 单类约 935 行 → 按控件族拆分 builder。
  - `MarkdownLayout.cpp::LayoutBlocks` 189 行 → 按块类型分派。
  - `DockManager.cpp::HitTestChrome` 139 行 → 分区判定 + 查表。
  - `InputHandler.cpp::SetMode` 115 行 switch → `unordered_map` 分派。
- Markdown 递归解析无深度限制（`MarkdownParser.cpp:470`）→ 见 P0，归入安全/健壮性。

---

## 6. 优先修复建议

**P0（立即）**
1. `AnimationService::Tick` 局部拷贝 + `weak_ptr`（#1）
2. `DragDropService::AbortIfParticipant` 接入析构路径（#2）
3. `BindableProperty` 改 `weak_ptr` + RAII guard（#3、#4）
4. docking：Pin 边界、延迟销毁、序列化保真（#5、#6、#7）
5. 终端：ConPTY 进程创建、OSC 52/8 收敛、转义序列上限与 UTF-8 校验（#8–#14）

**P1（本迭代）**
6. `Event::Invoke` 去拷贝
7. DirectWrite 文本格式缓存
8. TerminalBuffer 环形化
9. Layout：Grid 清零 + `isfinite` 防护
10. 拆分 `ElementBuilder` 与其余超长函数

**P2（随手工地）**
11. `parser/` → `style/`，皮肤外置
12. 属性系统边界写入规范文档

---

## 7. 修复状态（2026-09-19 实施）

### 已修复

| 项 | 文件 | 修复内容 |
|---|---|---|
| #1 | `animation/AnimationService.cpp` | `Tick()` 不再用 `std::move` 整体覆盖 `m_animatingElements`（会丢掉本帧新注册的动画）；改为只移除已结束的元素。遍历前逐个确认元素仍在活跃列表中，避免前一个控件销毁后一个控件导致的 UAF |
| #2 | `dnd/DragDropService.h` | `IDragSource` / `IDropTarget` 析构中自动调用 `AbortIfParticipant()`，补齐该函数的零调用点问题，彻底消除"拖拽中销毁元素 → 悬垂指针" |
| #3/#4 | `core/BindableProperty.h` | `m_updating` 改为 RAII `UpdateGuard`（异常/提前返回时必定复位）；保留 `Object& m_owner`（由 UIElement 持有，成员析构先于基类析构，生命周期安全） |
| #5 | `controls/docking/DockManager.cpp` | `OnMouseDown` 中 Close/Pin/AutoHide 三处 `hr.paneIndex` 增加上界校验（原只判 `>= 0`） |
| #6 | `controls/docking/DockFloatWindow.cpp` | `WM_CLOSE` 改为 `PostMessage(WM_CUI_FLOAT_CLOSED)`，关闭回调不再在窗口过程栈帧内 `delete` 自身（原路径会在对象释放后继续执行 `WM_DESTROY`） |
| #7 | `controls/docking/DockLayoutSerializer.cpp` | Load 时清空 `m_autoHide` 并复位 `pane.autoHide`；新增 autoHide 的保存/解析；未被任何 slot 认领且非浮动的 pane 回收到 Center，避免布局丢失 |
| #8 | `controls/terminal/ConPtyBackend.cpp` | `CreateProcessW` 显式传 `lpApplicationName`；可执行文件与参数分离保存并按 Windows 规则加引号转义，杜绝路径劫持/参数注入 |
| #9/#10 | `controls/terminal/InputHandler.cpp` | OSC 52 增加剪贴板策略开关（**读取默认禁止**，防外传）；OSC 8 超链接加协议白名单（`http/https/mailto`） |
| #11/#12/#13 | `controls/terminal/EscapeSequenceParser.cpp` | OSC/DCS 增加 64KB 长度上限；SOS/PM/APC 无终止符时按上限强制回到 Ground；UTF-8 增加 overlong / 代理区 / > U+10FFFF 校验；`m_collect` 累积加溢出保护 |
| #14 | `controls/terminal/ConPtyBackend.cpp` | 写队列增加 4MB 上限（背压，超限丢弃最旧块）；`Stop()` 中对子进程先等待 2s、超时 `TerminateProcess`，避免残留进程 |
| Markdown DoS | `text/markdown/MarkdownParser.cpp` | 引用块递归解析增加最大深度 24，超限降级为普通段落 |
| Spring 终值 | `animation/AnimationService.cpp` | `Spring` 缓动在 `t>=1` 时显式返回 1.0（原约 1.002，终值过冲） |
| Event O(n²) | `core/Event.h` | handler 列表改为 `shared_ptr` + 写时复制；`Invoke` 只做一次引用计数递增；`Connection` 增加共享 `alive` 标志，保证 `Disconnect` 后即使有旧快照也不会再触发。复杂度 O(n²) → O(n) |
| Grid 粘滞 | `layout/Layout.cpp` | `ArrangeGrid` 的 Auto 列/行在计算前清零（原来沿用 Grid 定义里的历史 `actualWidth`，导致只增不减） |
| NaN 传播 | `layout/Layout.cpp` | 新增 `SanitizeFloat/SanitizeSize/SanitizeRect`，在 `MeasureElement` / `ArrangeElement` / `MeasureGrid` / `ArrangeGrid` 入口与出口收敛非有限值 |
| 逐帧创建格式 | `render/GraphicsContext.cpp` | `MeasureText` 与 `CreateTextLayout` 改用按 (factory, 字体, 字号, 字重/样式/拉伸, 换行/对齐) 分区缓存的 `IDWriteTextFormat`，不再每次调用 `factory->CreateTextFormat` |

### 未修复（需专门设计，风险/收益比低）

| 项 | 说明 |
|---|---|
| TerminalBuffer `Reflow` 24MB 拷贝 | 需改为环形缓冲 + 增量重排，是滚动缓冲核心重构，建议独立任务并配单元测试 |
| Markdown `LayoutBlocks` 189 行拆分 | 纯重构，无功能收益 |
| `ElementBuilder` 935 行拆分 | 涉及 DSL 公共 API，需配套迁移方案 |
| `InputHandler::SetMode` 115 行 switch | 纯可读性重构 |
| `parser/StyleManager.cpp` 目录改名 | 影响 include 路径与工程配置，需同步改造 |
| 三套属性系统合并 | 架构级决策，先写规范再动代码 |
