import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import net from "node:net";
import process from "node:process";
import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { z } from "zod";

function discoverPipeName() {
  const configured = process.env.CUI_PIPE_NAME || process.argv[2];
  if (configured) return configured;

  const candidates = fs.readdirSync(os.tmpdir())
    .filter((name) => /^cui-mcp-.*\.pipe$/i.test(name))
    .map((name) => {
      const filePath = path.join(os.tmpdir(), name);
      try {
        return { filePath, mtime: fs.statSync(filePath).mtimeMs };
      } catch {
        return null;
      }
    })
    .filter(Boolean)
    .sort((left, right) => right.mtime - left.mtime);

  for (const candidate of candidates) {
    try {
      const value = fs.readFileSync(candidate.filePath, "utf8").trim();
      if (value.startsWith("\\\\.\\pipe\\")) return value;
    } catch {
      // The session can disappear while a window is shutting down.
    }
  }
  throw new Error("No CUI pipe session found; set CUI_PIPE_NAME or pass the pipe name as argv[2]");
}

function callCui(request) {
  return new Promise((resolve, reject) => {
    let pipeName;
    try {
      pipeName = discoverPipeName();
    } catch (error) {
      reject(error);
      return;
    }

    const socket = net.createConnection(pipeName);
    let response = "";
    let settled = false;
    const finish = (callback, value) => {
      if (settled) return;
      settled = true;
      socket.destroy();
      callback(value);
    };

    socket.setTimeout(15000, () => finish(reject, new Error("CUI request timed out")));
    socket.on("connect", () => socket.end(`${JSON.stringify(request)}\n`));
    socket.on("data", (chunk) => { response += chunk.toString("utf8"); });
    socket.on("end", () => {
      try {
        finish(resolve, JSON.parse(response));
      } catch (error) {
        finish(reject, new Error(`Invalid CUI response: ${error.message}`));
      }
    });
    socket.on("error", (error) => finish(reject, error));
  });
}

function result(value) {
  return {
    content: [{ type: "text", text: JSON.stringify(value) }],
    isError: value?.success === false
  };
}

const server = new McpServer({ name: "cui-mcp-server", version: "0.2.0" });

server.tool("ui_inspect", "Inspect the current CUI semantic UI tree.", {}, async () => result(await callCui({ op: "inspect" })));
server.tool("ui_find", "Find a CUI element by stable automation id.", { id: z.string().min(1) }, async ({ id }) => result(await callCui({ op: "find", id })));
server.tool("ui_wait_for", "Wait until an element with the stable automation id exists.", {
  id: z.string().min(1),
  timeout_ms: z.number().int().min(1).max(10000).optional()
}, async ({ id, timeout_ms }) => result(await callCui({ op: "wait_for", id, timeout_ms })));
server.tool("ui_get_property", "Read an exposed property from a CUI element.", {
  id: z.string().min(1), property: z.string().min(1), version: z.number().int().nonnegative().optional()
}, async ({ id, property, version }) => result(await callCui({ op: "get_property", id, property, version })));
server.tool("ui_set_property", "Set an exposed property on a CUI element.", {
  id: z.string().min(1), property: z.string().min(1), value: z.union([z.string(), z.number(), z.boolean()]), version: z.number().int().nonnegative().optional()
}, async ({ id, property, value, version }) => result(await callCui({ op: "set_property", id, property, value, version })));
server.tool("ui_type", "Type text into a TextBox by stable automation id.", {
  id: z.string().min(1),
  text: z.string().min(1).max(4096),
  replace: z.boolean().optional(),
  version: z.number().int().nonnegative().optional()
}, async ({ id, text: value, replace, version }) => result(await callCui({ op: "type", id, text: value, replace, version })));server.tool("ui_invoke", "Invoke a supported action on a CUI element.", {
  id: z.string().min(1), action: z.enum(["click"]), version: z.number().int().nonnegative().optional()
}, async ({ id, action, version }) => result(await callCui({ op: "invoke", id, action, version })));

await server.connect(new StdioServerTransport());
console.error("cui-mcp-server connected");

