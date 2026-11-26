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

`recommend_fix` requires application, control and framework investigation coverage, actual source
reads at all three levels, runtime/source parity caveats and per-item validation plans.
`ui-profiler/recommendations/v2` distinguishes application `propose` from native `recommend` entries.
Native entries stay visible as `target-workflow-required`; their MUX-relative scope is confined to
`controls\dev` or `dxaml\xcp`. This does not authorize an app trial or claim native candidate execution.
Legacy v1 reports remain readable.

`scripts\Start-RecommendationTrials.ps1` creates new independent app/native ADE proposals from
that report. `NativeTrials.psm1` enforces exact-hash approvals, target-aware source scopes,
source-built native baseline/candidate pairs and serialized execution. `NativeRuntime.psm1`
owns fixed native build plans and private runtime/resource staging; `Measure-NativeTrial.ps1`
is the trusted native check. The app check remains separate. Discovery inputs stay immutable.
Source preflight decodes NUL-delimited Git listings as UTF-8 independently of the caller's
console encoding, preserving non-ASCII filenames and excluded sample-submodule bindings.

Every application/propose id receives an independent
ADE coding subagent/session/workstream, explicit approved file scope and original-source working copy.
There is no two-file cap and no cumulative patch import. A shared frozen baseline/analysis/native
deployment is reused; builds/profiling are sequential with one candidate budget per approved id.
`recommendations-report.md`/`.json` retain every recommendation/status and hash-bound per-trial
multidimensional comparison, including failures/insufficient evidence, not only winning fixes.

`scripts\Start-Investigation.ps1 -Phase Investigate -Directory <new-report-directory>
-SourceInvestigation <existing-v3-investigation>` runs the actual three-level expert against
existing hash-verified baseline/analysis evidence. It takes zero captures, authorizes no edits and
never changes the sealed source investigation. Its read-only context supports `-Phase Report` only.

The servers require the Python dependencies in `requirements.txt`; the analyzer itself uses
only the standard library. Tools run with the engineer's rights, **not inside an OS sandbox**.
