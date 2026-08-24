# AutoGuard 设计规格

## 1. 项目目标

AutoGuard 是基于 CUI Fluent Builder UI 框架的 Windows 启动项管理工具，用于让系统启动与登录阶段的持久化行为透明、可分析、可控且可恢复。

首版采用模块化单体架构：UI、扫描器、风险分析器、管理器和备份记录模块位于同一进程，但通过接口隔离，后续可将需要管理员权限的管理操作拆分为提权助手或服务。

## 2. 产品边界

### 2.1 支持的启动入口

- 当前用户和本地计算机注册表 `Run`、`RunOnce`
- 当前用户和公共启动文件夹
- Windows 计划任务（Task Scheduler COM API，递归枚举任务文件夹和 Exec Action）
- Windows 服务和驱动服务
- `Active Setup`
- `Winlogon` 启动链
- IFEO 映像劫持
- WMI 永久事件订阅
- COM/CLSID/InprocServer32 常见劫持入口
- 常见 Explorer、Shell 和浏览器扩展入口

### 2.2 管理能力

- 扫描、刷新、取消扫描
- 禁用、恢复和删除启动项
- 批量选择、批量禁用和批量恢复
- 打开文件位置、打开注册表位置、查看任务或服务详情
- 查看 Authenticode 签名、发布者、文件描述和哈希
- 所有修改前自动创建备份
- 从备份恢复原始配置
- 保留操作历史并支持撤销最近操作

### 2.3 安全约束

- 默认只读扫描，不自动修改系统
- 默认优先禁用，不直接删除
- 系统关键项默认禁止直接删除
- 高风险管理操作必须管理员确认
- 不自动启动或加载被扫描文件
- 扫描失败、权限不足和入口不可访问必须显式展示
- 任何修改必须记录入口、原始值、目标值、时间、操作者权限和相关文件哈希

## 3. 风险模型

每个 `StartupEntry` 具有以下分析字段：

- 入口类型、作用域、显示名称、原始命令和解析后的可执行路径
- 当前状态：启用、禁用、失效、受保护、未知
- 文件存在性、文件大小、最后修改时间和 SHA-256 哈希
- 签名状态：已签名且有效、已签名但无效、未签名、无法验证、不适用
- 签名发布者、产品名和文件描述
- 是否为系统目录、用户目录、临时目录、隐藏目录或可疑可写目录
- 风险等级：安全、注意、可疑、高风险
- 风险原因列表和证据来源

风险评分必须是可解释的规则系统，不依赖网络服务。规则至少覆盖：

- 文件不存在或命令目标失效
- 未签名、签名无效或发布者与路径异常不匹配
- 位于临时目录、下载目录、用户可写隐藏目录
- 使用脚本解释器、`rundll32`、`regsvr32`、PowerShell 等间接执行方式
- IFEO、WMI、Winlogon、COM 等高隐蔽入口
- 命令参数包含可疑混淆、远程路径或异常扩展名
- 已知系统组件和可信发布者的降权处理

## 4. 数据模型

核心模型位于 `AutoGuard/model`：

- `StartupLocation`：启动入口类型及作用域
- `StartupEntry`：启动项统一记录，包含原始数据、分析结果、管理状态和唯一 ID
- `RiskLevel`：风险等级
- `SignatureStatus`：数字签名状态
- `StartupStatus`：启用、禁用、失效、受保护、未知
- `ScanSummary`：扫描统计、耗时、失败项和风险汇总
- `OperationRecord`：管理操作、备份引用、结果和错误信息

扫描器只负责发现和读取，不直接改变系统；管理器只接收统一的 `StartupEntry` 和管理命令。所有系统入口扫描必须使用对应 Windows 原生 API、COM 接口或文件系统 API，不通过命令行工具解析文本输出。

## 5. 模块划分

```text
AutoGuard/
├─ AutoGuard.vcxproj
├─ main.cpp
├─ AutoGuardApp.h
├─ AutoGuardApp.cpp
├─ model/
│  ├─ StartupEntry.h
│  ├─ StartupLocation.h
│  ├─ RiskLevel.h
│  ├─ SignatureStatus.h
│  ├─ StartupStatus.h
│  ├─ ScanSummary.h
│  └─ OperationRecord.h
├─ scanner/
│  ├─ StartupScanner.h
│  ├─ RegistryScanner.h/.cpp
│  ├─ StartupFolderScanner.h/.cpp
│  ├─ ScheduledTaskScanner.h/.cpp
│  ├─ ServiceScanner.h/.cpp
│  ├─ PersistenceScanner.h/.cpp
│  └─ SignatureScanner.h/.cpp
├─ manager/
│  ├─ StartupManager.h/.cpp
│  ├─ BackupManager.h/.cpp
│  ├─ ElevationManager.h/.cpp
│  └─ OperationHistory.h/.cpp
├─ pages/
│  ├─ DashboardPage.cpp
│  ├─ StartupEntriesPage.cpp
│  ├─ RiskCenterPage.cpp
│  ├─ DisabledEntriesPage.cpp
│  ├─ HistoryPage.cpp
│  └─ SettingsPage.cpp
└─ resources/
   └─ app.rc
```

## 6. UI 设计

所有应用层 UI 使用 CUI Fluent Builder 链式 API；窗口、根节点和页面组合遵循现有项目的链式写法，不在应用层混用旧式 setter 写法。

主窗口采用左侧导航 + 顶部标题栏 + 内容区域 + 底部状态栏布局：

- **总览**：启动项总数、风险项数量、失效项数量、已禁用数量、最近扫描信息和快速操作
- **启动项**：支持入口筛选、风险筛选、状态筛选、搜索、排序、多选和详情侧栏
- **风险中心**：按风险等级和风险原因聚合，提供进入详情和批量处理入口
- **已禁用**：展示备份关联，支持恢复和永久删除
- **操作历史**：展示操作时间、对象、动作、结果，支持撤销
- **设置**：扫描范围、哈希开关、签名验证开关、备份目录、主题和语言

扫描在后台线程执行，UI 线程只接收快照和进度事件。长任务必须支持取消、失败分项和最后一次成功快照保留。

## 7. 权限与修改策略

- 普通用户权限可以扫描当前用户入口及可读取的系统入口。
- 需要管理员权限时，界面明确标记“需要管理员权限”，不静默失败。
- 首版使用提权重启/提权操作入口完成管理动作，接口设计保留后续独立助手或服务的替换空间。
- 禁用策略按入口类型实现：注册表项移动到 AutoGuard 备份区、启动文件夹项移动或改名、计划任务禁用、服务停止并设置禁用、特殊入口按安全策略写入备份后处理。
- 删除操作必须在完成备份、二次确认和风险提示后执行。

## 8. 备份与恢复

备份记录采用版本化 JSON 文件，包含：

- AutoGuard 版本、备份 ID、创建时间
- 启动项唯一 ID、入口类型和作用域
- 原始位置、原始值、修改前状态
- 关联文件路径和修改前哈希
- 操作类型、结果和错误信息

恢复时重新验证目标文件和入口状态；若文件已被替换、路径变化或哈希不匹配，必须提示用户，不自动覆盖新对象。

## 9. 首批实施范围

第一批只实现可运行的工程骨架和安全扫描基础：

1. 创建 `AutoGuard` Visual Studio 工程并接入 `CUI.Core`。
2. 创建 Fluent Builder 主窗口、导航、总览页和启动项列表页。
3. 完成统一数据模型、扫描接口、任务进度和取消机制。
4. 实现注册表 `Run/RunOnce`、启动文件夹、计划任务、服务扫描。
5. 先以只读方式展示结果，管理动作接入统一命令接口并保留备份扩展点。

第二批再实现特殊持久化入口、签名验证、风险评分、禁用/恢复和操作历史。

## 10. 验收标准

- `AutoGuard` Debug x64 工程可独立构建并启动。
- 所有应用层 UI 代码使用 Fluent Builder 链式方法。
- 首批入口扫描结果能统一显示名称、入口、命令、路径、状态和风险基础字段。
- 扫描异常不会导致 UI 崩溃，能够显示失败原因。
- 扫描可取消，页面不会阻塞。
- 管理模块接口不绕过备份层直接修改系统。
- 未经用户确认不会修改或删除启动项。
- 之后扩展特殊入口不需要改动主窗口和列表页的数据消费接口。

