# Tools

Put scripts here that help the coding agent. The runtime lists each `.ps1`, `.py` or `.cmd` file in
the agent's instructions, with the first comment line as the description. The agent runs them with the
shell when they help.

- Write the first comment line as a one-line description.
- Make the script safe to run more than once.
- A tool is trusted code. It runs with the engineer's rights.

To add an MCP server, create `pack\mcp.json`:

```json
{
  "mcpServers": {
    "search-index": { "command": "pwsh", "args": ["-File", "${PACK_DIR}/tools/index-server.ps1"] }
  }
}
```

`${PACK_DIR}` is replaced with this pack folder. The runtime allows only servers that are listed here.

`repo-summary.ps1` is the starter tool.
