# CUI MCP Bridge

## Goal

Expose the CUI semantic UI tree to MCP clients without exposing C++ pointers, HWND values, or arbitrary callbacks.

## Process Model

```text
MCP Client
    |
    | MCP JSON-RPC over stdio
    v
cui-mcp-server
    |
    | per-window Windows Named Pipe
    v
AutomationPipeServer
    |
    | AutomationHost + Window::InvokeOnUiThread
    v
CUI UIElement tree
```

The MCP server is a separate process. It owns MCP protocol parsing and tool schemas. CUI owns element lookup, permissions, state validation, and UI-thread execution.

## Connection

`AutomationPipeServer` creates a per-window pipe named like:

```text
\\.\pipe\CUI.<processId>.<windowHandle>
```

It also writes the pipe name to a short-lived session file under the Windows temporary directory:

```text
%TEMP%\cui-mcp-<processId>-<threadId>.pipe
```

The Node server accepts `CUI_PIPE_NAME`, or the pipe name as `argv[2]`. If neither is provided, it discovers the newest valid session file automatically. This keeps pipe names out of prompts and avoids exposing window handles to the AI.

## MCP Tools

- `ui_inspect`: return the semantic UI tree and snapshot version.
- `ui_find`: resolve a stable automation ID and return current metadata.
- `ui_wait_for`: poll until an element ID exists, with a maximum timeout of 10 seconds.
- `ui_type`: replace or append up to 4096 bytes of text in a `TextBox`.
- `ui_get_property`: read an allowed property.
- `ui_set_property`: write an allowed property after validation.
- `ui_invoke`: execute a registered action; currently only `click` is exposed.

## Request Safety

1. AI sends stable string IDs, never raw pointers or HWND values.
2. Every operation resolves the ID again; no UIElement pointer is persisted across requests.
3. UI access is dispatched to the owning UI thread through `Window::InvokeOnUiThread`.
4. Mutations can require an expected snapshot version and are rejected when stale.
5. Hidden, disabled, unsupported, or invalid operations are rejected.
6. `ui_wait_for` sleeps on the pipe worker thread and performs short, repeated ID lookups; it never holds a UI pointer while waiting.
7. `ui_type` accepts only `TextBox` elements, rejects read-only controls, and applies keyboard focus through the owning `Window`.
8. Request and execution timeouts are bounded; the MCP side caps waits at 10 seconds.
9. The endpoint is stopped before the owning `Window` destroys its automation host.

## Current Scope

The first implementation exposes inspection, lookup, property access, and `click`. Text entry, selection, scrolling, and control-specific actions should be added only through explicit allowlisted action handlers; arbitrary C++ method names are never accepted from the AI.



