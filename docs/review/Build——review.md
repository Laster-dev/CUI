# Build / 仓库工程卫生 —— 代码审查报告

- 审查日期：2026-09-19
- 范围：`CUI.slnx`、全部 `.vcxproj`、`.gitignore`、LICENSE、`README.md`
- 方式：只读静态审查

---

## 1. P1 — 构建配置漂移

| # | 问题 | 涉及工程 | 后果 | 建议 |
|---|---|---|---|---|
| 1 | **`/utf-8` 仅部分配置开启** | AutoGuard、Demo 仅 2/4 配置；Calc、CUI、CUI.Gallery、RegeditPlus、Patcher 需逐一核对 | 未开启配置下中文字符串/注释被按本地代码页解析 → **中文乱码**，且 Debug/Release 行为不一致 | 全部工程 × 全部配置统一加 `/utf-8`；建议同时加 `/Zc:__cplusplus`、`/permissive-` |
| 2 | **工具集混用（v143 / v145）** | 跨工程 | SDK/工具集版本不一致，产物 ABI 与行为差异，协同开发易踩坑 | 统一 `PlatformToolset`，写入 `Directory.Build.props` |
| 3 | **`OutDir` 写死 x64** | EverythingNEO | Win32/ARM64 配置产物输出到同一目录，**互相覆盖** | 改为 `$(Platform)` / `$(Configuration)` 参数化 |
| 4 | 缺少统一属性表 | 全仓库 | 每个 vcxproj 各自维护编译选项，必然继续漂移 | 新增 `Directory.Build.props` / `Directory.Build.targets` 集中管理语言标准、字符集、警告级别、SDL、输出目录 |

---

## 2. P1 — 合规与文档

| # | 问题 | 后果 | 建议 |
|---|---|---|---|
| 5 | **仓库无 LICENSE** | 法律上默认"保留所有权利"，他人无法合法使用/贡献 | 补 `LICENSE`（与 README 声明的许可证一致） |
| 6 | README 中的许可/链接为 **404** | 文档失真 | 修正链接；对外部链接加 CI 死链检查 |
| 7 | `tools/cui-mcp-server` 依赖的 `AutomationPipeServer` 代码 0 命中（详见 `tools——review.md`） | 文档承诺的功能不可用 | 补实现或下架文档 |

---

## 3. P2 — 建议加固项

- **警告级别**：统一 `/W4`（或 `/W3` + 关键警告升错），并列出允许的白名单警告编号；避免 `/WX` 与第三方头冲突时可局部 `#pragma warning(push/pop)`。
- **SDL / 安全编译开关**：确认全工程开启 `/sdl`、`/GS`、`/guard:cf`（若兼容），尤其是解析外部输入的工程（Patcher、EverythingNEO、AutoGuard、终端）。
- **预编译头 /  unity build**：CUI.Core 体量大，可评估编译耗时优化。
- **CI**：当前似无 CI。建议加最小流水线：构建（多配置）、编译警告统计、CRLF 一致性检查、脚本 `--dry-run` 冒烟、死链检查。
- **`.gitignore`**：确认忽略 `*.user`、`*.suo`、构建中间目录、`Everything.db` 等运行时产物。
- **版本/变更记录**：建议加 `CHANGELOG.md`，把本次审查的 P0 修复记录进去。

---

## 4. 优先修复建议

**P1（立即）**
1. 全工程全配置补 `/utf-8`（#1）
2. 统一 `PlatformToolset`（#2）
3. `OutDir` 参数化（#3）
4. 新增 `Directory.Build.props` 集中编译选项（#4）
5. 补 LICENSE + 修正 README 链接（#5、#6）

**P2**
6. 统一警告级别与 SDL 开关
7. 建立最小 CI（构建 + 静态检查 + 死链）
8. `.gitignore` 与 `CHANGELOG.md` 完善
