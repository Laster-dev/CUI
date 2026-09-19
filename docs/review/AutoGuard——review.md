# AutoGuard —— 代码审查报告

- 审查日期：2026-09-19
- 工程路径：`AutoGuard/`
- 规模：11 源文件 / 11 头文件（24 文件）
- 方式：只读静态审查

---

## 1. 范围与架构

| 文件 | 职责 |
|---|---|
| `scanner/StartupScanner.cpp`（**2672 行**） | 枚举启动项：注册表 Run/RunOnce、计划任务、服务、启动目录、WMI 等 |
| `manager/StartupManager.cpp` | 启动项的启用/禁用/删除/导出，计划任务调度，`schtasks` 调用 |
| 其余 `*.cpp` | UI 页面、列表渲染、托盘与自启动管理 |

COM 使用：`CoInitializeEx`；任务计划通过 `schtasks.exe` 命令行 + Taskschd 混合方式。

---

## 2. P0 — 数据破坏

| # | 问题 | 位置 | 后果 | 建议 |
|---|---|---|---|---|
| 1 | `DeleteFileW(entry.command)` 删除启动项时连带删除目标文件 | `manager/StartupManager.cpp:167` | **误删程序本体**，不可逆数据丢失 | 仅删除注册表值 / 计划任务 / 快捷方式；绝不删除 command 指向的可执行文件 |
| 2 | 仅凭 PE 资源 `CompanyName` 标记 `"(Verified)"` | `scanner/StartupScanner.cpp` | Authenticode 可伪造，安全结论**误导用户** | 改用 `WinVerifyTrust` 校验签名链与时间戳；无签名则显示"未签名" |

---

## 3. P0 — 安全

| # | 问题 | 位置 | 后果 | 建议 |
|---|---|---|---|---|
| 3 | `schtasks /run` 命令行字符串拼接 | `manager/StartupManager.cpp:331` | 任务名可控 → **命令注入** | 参数白名单 + 转义；或改用 Taskschd COM 接口（`ITaskFolder`/`IRegisteredTask`） |
| 4 | CSV 导出未转义 | `manager/StartupManager.cpp:310` | 以 `=+-@` 开头的字段在 Excel 中被执行 → **公式注入** | RFC4180 转义（引号加倍、整体加引号），危险前缀前加 `'` |
| 5 | 在 STA 线程内调用 `CoInitializeEx(MULTITHREADED)` 且不检查返回值 | `manager/StartupManager.cpp:98` | 返回 `RPC_E_CHANGED_MODE`，COM 单元契约被破坏，后续 COM 调用行为未定义 | 检查返回值并据此选择同步/异步路径；统一单元模型 |

---

## 4. P1 — 正确性

| 问题 | 位置 | 说明与建议 |
|---|---|---|
| scope 字符串比较恒为 false | `manager/StartupManager.cpp:39` | 分支永不进入（逻辑死代码），用户/机器级启动项切换实际失效。校正枚举与字符串映射 |
| 删除/禁用缺少事务与回滚 | `StartupManager.cpp` | 删除前应先备份（导出的启动项快照），支持一键还原 |
| 危险操作缺少二次确认与影响面说明 | UI 层 | 删除前明确展示"将删除注册表项 X，不影响程序文件" |

---

## 5. P1 — 可维护性

- `scanner/StartupScanner.cpp` **2672 行**，混合了枚举、解析、评分、PE 资源读取四类职责。
  - 建议按扫描源拆分：`RegistrySource.cpp` / `TaskSchedulerSource.cpp` / `ServiceSource.cpp` / `StartupFolderSource.cpp` / `WmiSource.cpp`，统一 `IStartupSource` 接口。
  - 评分/判定逻辑独立为 `StartupRiskEvaluator.cpp`。
- 扫描为同步阻塞或半同步，UI 需在后台线程并支持取消（确认当前实现）。
- 缓存：`CompanyName` 等 PE 资源读取应做路径级缓存，避免重复打开文件。

---

## 6. P2 — 工程质量

- `AutoGuard.vcxproj` **仅在 2/4 配置中带 `/utf-8`** → 其余配置下中文字符串（UI 文案、注册表路径含中文）乱码。全配置统一补 `/utf-8`。
- 工具集与其它工程混用（v143 / v145），建议统一写入 `Directory.Build.props`。
- 缺少操作审计日志：启动项变更应记录时间、来源、旧值，便于用户追溯与还原。

---

## 7. 优先修复建议

**P0（立即，阻断发布）**
1. 移除 `DeleteFileW(entry.command)`（#1）
2. 签名判定改用 `WinVerifyTrust`（#2）
3. `schtasks` 参数转义或改 COM（#3）
4. CSV 转义（#4）
5. `CoInitializeEx` 返回值处理（#5）

**P1（本迭代）**
6. 修正 scope 比较死代码（#6）
7. 删除操作加备份与回滚
8. 拆分 `StartupScanner.cpp`

**P2**
9. 补 `/utf-8` 全配置
10. 增加操作审计日志
