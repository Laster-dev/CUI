# tools（含 cui-mcp-server）—— 代码审查报告

- 审查日期：2026-09-19
- 路径：`tools/`
- 规模：6 文件（2 `.ps1`、2 `.json`、1 `.js`/`.mjs` 等）
- 方式：只读静态审查

---

## 1. 范围

| 文件 | 职责 |
|---|---|
| `tools/cui-mcp-server/server.mjs` | MCP 服务端，用于向外部工具暴露 CUI 自动化能力 |
| `*.ps1` | 构建/环境辅助脚本 |
| `*.json` | MCP / 编辑器配置 |

---

## 2. P0/P1 — 文档与实现脱节

| # | 级别 | 问题 | 后果 | 建议 |
|---|---|---|---|---|
| 1 | **P1（对外承诺失效）** | `cui-mcp-server/server.mjs` 依赖的 C++ `AutomationPipeServer` 在整个代码库中 **0 命中** | MCP 能力**实际不可用**，文档宣称的功能无法兑现；使用者按文档接入后报错 | 二选一：① 补实现 `AutomationPipeServer` 并接入；② 下架/降级文档，明确标注"规划中" |
| 2 | P2 | `docs/CUI-MCP-Bridge.md` 描述的接口与 `server.mjs` 实际实现需逐条比对 | 文档与代码漂移 | 建立文档—接口一致性检查（CI 校验导出的工具清单） |

---

## 3. P2 — 工程质量

- PowerShell 脚本需确认：执行策略处理、错误终止（`$ErrorActionPreference = 'Stop'`）、路径不使用硬编码盘符。
- MCP 服务端缺少：请求参数校验、超时、错误码规范、日志级别控制。
- 建议为 MCP 工具增加"只读/写入"标记，写入类工具默认需要显式确认。

---

## 4. 优先修复建议

**P1（本迭代）**
1. 补 `AutomationPipeServer` 实现或下架 MCP 文档（#1）
2. 文档—接口一致性核对（#2）

**P2**
3. PowerShell 脚本加 `Stop` 错误策略与相对路径
4. MCP 服务端加参数校验、超时与错误码
