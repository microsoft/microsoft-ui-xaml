# Harness adapters and evidence tools

`Harness-Mcp.py` exposes granular investigation tools to the real controller:
`harness_contracts`, `begin`, `inspect_application`, `preview_application`, `inspect_ui`,
`act_on_ui`, `freeze_scenario`, `close_preview`, `measure_baseline`, `analyze_baseline`,
`recommend_fix`, `propose_fix(recommendation_id)`, `recommendation_report` and `investigation_status`.
`Invoke-HarnessTool.ps1` enforces deterministic input, authorization, state and integrity gates.
The agent chooses a plan; these scripts are tool implementations, not a substitute agent.

`Evidence-Mcp.py` is a separate, read-only **team-plugin** server over a run's hash-verified
derived analysis. It provides capability discovery, metric paging and JSON-pointer drilldowns.
The existing MUX WinUIProfiler MCP remains unchanged and provides original recording queries.
Both servers are declared in each independent trial's `pack\mcp.json`; the outer mutating harness
server is registered separately in `plugin.json` and is never granted to the coding agent.

The per-run pack includes `knowledge\harness-contracts.md`, copied from `CONTRACTS.md` and
hash-bound in context/approval. The controller and expert use it to choose legal pivots;
deterministic checks remain authoritative.

PowerShell helpers own builds, source isolation, owned-process UI Automation, captures,
evidence exports, actual expert invocation and ADE handoff. The coding session has no shell
permission and must not execute them itself. Failed/reserved captures are not idempotent retries.
`Compare-Evidence.ps1` supports offline reanalysis into a fresh directory without new captures.

`recommend_fix` retains the complete ranked expert report. Every actionable id receives an independent
ADE coding subagent/session/workstream, explicit approved file scope and original-source working copy.
There is no two-file cap and no cumulative patch import. A shared frozen baseline/analysis/native
deployment is reused; builds/profiling are sequential with one candidate budget per approved id.
`recommendations-report.md`/`.json` retain every recommendation/status and hash-bound per-trial
multidimensional comparison, including failures/insufficient evidence, not only winning fixes.

The servers require the Python dependencies in `requirements.txt`; the analyzer itself uses
only the standard library. Tools run with the engineer's rights, **not inside an OS sandbox**.
