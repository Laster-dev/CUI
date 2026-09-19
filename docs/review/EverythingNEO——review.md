# EverythingNEO —— 代码审查报告

- 审查日期：2026-09-19
- 工程路径：`EverythingNEO/`
- 规模：10 源文件 / 10 头文件（22 文件）
- 方式：只读静态审查

---

## 1. 范围与架构

| 文件 | 职责 |
|---|---|
| `EverythingEngine.cpp` | 查询引擎：USN 线程管理、查询缓存栈、前缀过滤、结果集 |
| `UsnWatcher.cpp` | USN 日志监听，增量更新索引树 |
| `VolumeIndexer.cpp` | 卷扫描：MFT 枚举 / USN 遍历 / `FindFirstFile` 回退 |
| `StringArena.cpp` | 字符串池，文件名偏移量存储，`Everything.db` 二进制快照读写 |
| （UI）虚拟列表 ListView | 大量结果虚拟化渲染 |
| `Everything.db` | 二进制索引快照 |

技术要点：NTFS USN 日志 / MFT 枚举、`FindFirstFile` 回退路径、`StringArena` 字符串池、虚拟列表。

---

## 2. P0 — 数据破坏与死锁

| # | 问题 | 位置 | 后果 | 建议 |
|---|---|---|---|---|
| 1 | 分块分配留空洞，save/load 后 `name_offset` 错位 | `StringArena.cpp:25-27, 75-89` | **数据库重载后所有文件名错乱**，索引彻底失效（最致命） | 序列化写入 `(ChunkId, ChunkOffset)` 二元组而非全局偏移；或按 chunk 独立序列化并加校验和 |
| 2 | USN 线程在自身线程上下文 `join()` | `EverythingEngine.cpp:199` | 退出时**必然死锁**，进程无法退出 | 停止标志 + 控制线程 join；加超时与 detach 兜底 |
| 3 | 共享锁下写 `m_queryCacheStack` | `EverythingEngine.cpp:226, 386` | 数据竞争、迭代器失效、缓存错乱 | 写路径升级 `unique_lock`，或改为无锁结构 |
| 4 | 重命名产生重复节点 | `UsnWatcher.cpp` | 同一文件出现多条结果，索引膨胀 | 以 `FileReferenceNumber` 为主键做 upsert |
| 5 | 删除不级联子节点 | `UsnWatcher.cpp` | 已删除目录下文件长期残留，结果指向不存在路径 | 删除时递归清理子树 |
| 6 | USN 记录无边界/版本校验 | `VolumeIndexer.cpp` | 越界读取、异常记录导致崩溃 | 校验 `RecordLength >= sizeof(USN_RECORD)` 且 `MajorVersion` 匹配 |
| 7 | 重解析点递归无深度限制 | `VolumeIndexer.cpp` | 符号链接环 → **无限递归 / 栈溢出** | 递归深度上限（如 32）+ 已访问集合 |

---

## 3. P1 — 正确性

| 问题 | 位置 | 说明与建议 |
|---|---|---|
| 前缀过滤漏结果 | `EverythingEngine.cpp:264` | 缓存命中路径下过滤条件未生效，结果不全。统一过滤入口，缓存键包含过滤参数 |
| `FindFirstFile` 回退路径一致性 | `VolumeIndexer.cpp` | 回退枚举与 USN 索引的节点主键需一致，否则重复插入 |
| 快照兼容性 | `Everything.db` | 二进制快照无版本号/魔数，结构变更后旧库静默损坏。加 `magic + version + checksum` |

---

## 4. P1 — 性能

- 查询缓存栈在锁内做线性扫描 → 大结果集下退化为 O(n)；建议改为 `unordered_map` + LRU。
- USN 增量更新逐条同步到 UI → 高频变更期 UI 卡顿；建议批处理 + 节流刷新。
- 虚拟列表需确认是否复用行容器与文本格式（同 CUI.Core 的 DirectWrite 缓存建议）。

---

## 5. P2 — 工程质量

- `EverythingNEO.vcxproj` 的 `OutDir` 写死 x64 → 改为 `$(Platform)` 参数化，否则 Win32/ARM64 配置产物互相覆盖。
- 索引损坏缺少自愈：建议启动时校验快照，失败则自动触发全量重建并提示用户。
- 缺少索引健康检查/重建入口（对用户可见的"重建索引"按钮）。

---

## 6. 优先修复建议

**P0（立即，阻断发布）**
1. `StringArena` 偏移量语义修正（#1）—— 修复后需**强制废弃旧 `Everything.db` 并重建**
2. USN 线程停止改为标志 + 控制线程 join（#2）
3. `m_queryCacheStack` 写路径独占锁（#3）
4. USN 重命名去重 + 删除级联（#4、#5）
5. USN 记录边界/版本校验 + 重解析点递归上限（#6、#7）

**P1（本迭代）**
6. 前缀过滤统一入口（#8）
7. `Everything.db` 加 magic/version/checksum
8. 查询缓存改哈希 + LRU，更新批处理节流

**P2**
9. `OutDir` 参数化
10. 索引损坏自愈与手动重建入口
