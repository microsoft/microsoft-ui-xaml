# ui-profiler ADE team plugin

ADE is an external prerequisite, not copied into the app or MUX. Generate/install the
shared runtime with the ADE platform's `setup.ps1`. This directory is the team plugin.
It uses only MUX WinUIProfiler, including its GUI's native SQLite metric persistence;
there is no alternative profiling backend. It works through Agency's `ui-profiler` plugin/`work` skill, or
the standalone Copilot controller below.

The current minimal adapter supports clean Git repositories containing unpackaged,
self-contained .NET WinUI apps. **The user supplies a natural-language scenario.**
A real planning agent inspects source and an explicitly authorized unrecorded preview,
chooses project/build/UI actions/selectors and freezes a validated internal replay plan.
There is no required scenario JSON; Gallery's checked-in JSON is only a regression fixture.
The harness owns build/deployment and external UI Automation; the app has no harness references.
Use an interactive Windows desktop. Keyboard actions refuse to run if the owned app loses focus.
For observed items exposing UIA SelectionItemPattern, `select` changes selection without sending
global keyboard input. It requires a real selection transition and records before/after state.
Choose a short investigation directory: WinUI's generated-code paths can exceed legacy path limits.

## Setup and use

Prerequisites are ADE, a licensed/authenticated Copilot CLI, Python with the official MCP SDK,
PowerShell 7, Git, a compatible .NET SDK/runtime and the app's normal build prerequisites.
Use a local Python environment; its activated `python` is also used by Agency's MCP server:

```powershell
python -m venv C:\Tools\ui-profiler-mcp
& C:\Tools\ui-profiler-mcp\Scripts\Activate.ps1
python -m pip install -r .\requirements.txt
.\scripts\ade.ps1 doctor
```

Install/register this team plugin through ADE's platform setup, and start Agency with this
environment active. For standalone preparation, supply separate input roots and the user's
words, with explicit preview and baseline/candidate recording consent:

```powershell
.\scripts\Start-Investigation.ps1 -Phase Prepare `
    -Directory C:\PerfRuns\gallery-treeview `
    -ApplicationRepository Q:\Repo\WinUI-Gallery `
    -Scenario 'Open Gallery, search for TreeView, then expand and collapse the basic sample.' `
    -KnowledgeRepository Q:\Repo\microsoft-ui-xaml `
    -ExpertHome C:\Tools\WinUI_Expert_Agent `
    -ProfilerCli C:\Tools\WinUIProfiler\WinUIProfiler.Cli.exe `
    -CaptureBackend Gui -ProfilerGui C:\Tools\WinUIProfilerGui\WinUIProfiler.App.exe `
    -Dotnet C:\Tools\dotnet\dotnet.exe `
    -AdeCopilotCli C:\Tools\Copilot\copilot.exe `
    -GitHubAccount <existing-licensed-account> `
    -AllowPreview -AllowMeasurements
```

New preparations default to **GUI capture** and require `-ProfilerGui`; missing or
changed GUI payloads fail without falling back to CLI capture. The harness launches
one owned GUI, drives its scoped UIA connect/snapshot/disconnect controls, and closes
it normally to drain post-recording SQLite writers. Its finalized SQLite is copied
byte-for-byte and hash-bound alongside GUI/target process identities and the action audit.
Process CPU/GPU/memory samples come from that database's `ProcessMetrics`, not chart
text or a separate sampler. Missing, lost, trimmed or mismatched evidence is rejected;
unavailable samples remain null with reasons, and measured zero stays zero.
CLI commands are still used for **read-only SQLite exports**, not GUI-mode recording.
Explicit `-CaptureBackend Cli` and old sealed CLI runs remain supported without rewriting history.
Use MUX's `tools\WinUIProfiler\scripts\Build-App.ps1` for the complete GUI deployment.
A managed-only `dotnet build` is insufficient: native metrics, connection and launcher
payloads must also be staged. Incomplete deployments are refused before a capture is reserved.

Build the CLI with MUX's existing `tools\WinUIProfiler\scripts\Build-Cli.ps1`; distribute
its entire output, not just the executable. Supply the actual Godly.Tools
`WinUI_Expert_Agent` bundle. Local RAG indexes its existing KB; no collectors,
global agent installation, scheduled tasks, or automatic refresh are used.
The selected existing GitHub credential is scoped to the harness's child processes, not a global account switch.
ADE uses its own task-local client state and the same explicitly selected account.
`-AdeCopilotCli` pins a compatible native CLI without changing the machine install;
Prepare checks SDK/server compatibility and authentication before spending a capture.
If client preflight fails after planning but before any native preflight or reserved capture,
`scripts\Restart-UnmeasuredInvestigation.ps1 -SourceInvestigation <old> -Directory <new>
-AdeCopilotCli <compatible-client>` creates an explicitly linked continuation. It preserves
the old request and byte-identical scenario, revalidates the original observed selectors and
unchanged app/profiler/replay inputs, and probes the repaired client. It launches no preview,
grants no measurement permission, and rejects any existing baseline directory or trial.
Replay timeouts are derived from the frozen step count and waits, including bounded
selector/state completion. The profiler's recording deadline also covers startup and
the final snapshot; a legal long plan is not cut off by a fixed 90-second replay limit.
Timeouts still fail the trial and retain its spent budget and diagnostics.

`Prepare` launches the real tool-using controller, not a script-defined Gallery flow.
Its bounded `ui-profiler-harness` MCP tools copy source, inspect projects/UI, freeze the
agent-generated plan, publish a release unpackaged deployment,
stages the complete evaluated build-output closure (including app PRI/compiled XAML),
records the scenario once, validates/hashes SQLite evidence, obtains the actual
read-only expert recommendation report from the full dimensional analysis, and creates an independent
ADE proposal/workstream for each application/propose item. Each later coding subagent starts in a fresh working
copy of the original source, not another trial's patch. One fix can span every explicitly approved
existing C#/XAML file it needs; all other tracked files remain protected.
Review the report, frozen scenario and each proposal's exact hash.
**Preview/measurement consent is not approval of a source change. The harness never approves for you.**

```powershell
.\scripts\ade.ps1 approve --run <runId> --hash <proposalSha256> `
    --runs-dir C:\PerfRuns\gallery-treeview\trials\<recommendationId>\ade-runs
.\scripts\Start-Investigation.ps1 -Phase Run -Directory C:\PerfRuns\gallery-treeview `
    -RecommendationId '<recommendationId>' `
    -GitHubAccount <existing-licensed-account>
.\scripts\Start-Investigation.ps1 -Phase Verify -Directory C:\PerfRuns\gallery-treeview
.\scripts\Start-Investigation.ps1 -Phase Report -Directory C:\PerfRuns\gallery-treeview
.\tests\Test-Harness.ps1
```

ADE owns a separate real Copilot coding subagent/session, exact-hash approval, isolated working copy,
single coding round, verification and provenance for **each application/propose** recommendation. Shell/URL access is disabled.
Its check builds/deploys that variant and runs the same scenario once.
Source-matched cached checks do not consume another capture. A failed or reserved
trial capture cannot retry. After approving multiple proposals, pass their ids as
`-RecommendationId @('first-id', 'second-id')` to run sequentially on the shared desktop.
The shared baseline recording/analysis and native MCP deployment are reused, not rebuilt/recorded
for each recommendation; only small per-trial packs and working copies are separate.
`-AllowMeasurements` authorizes one baseline and one candidate per independently approved recommendation
in the finite frozen report. It does not approve any source proposal.
Do not combine/apply the independent ADE diffs unless separately requested and verified.

`recommendations-report.md` and `recommendations-report.json` retain **all** recommendations,
source/evidence reasons, file scopes, risks, exact proposal/run ids and outcomes. Pending,
unapproved, insufficient-evidence and failed entries remain visible without invented results.
Every measured trial has `trials\<id>\multidimensional\comparison.md`, `.json` and `.csv` against
the SAME original baseline, covering all dimensions and regressions. There is no scalar winner score
and no claim that the separately tested patches work together.

## Three investigation levels

The actual WinUI expert investigates **application usage**, **control implementation** and
**core framework** in every new recommendation report. Both performance MCPs and actual source
reads at all three levels are required. The v3 report retains separate coverage, source/evidence
citations, runtime/source parity limitations, scoped recommendations and validation plans.
Every level uses the same deep question/time/CPU/memory/model-analysis/unverified/validation
layout. WinUIProfiler supplies measurements; the model can reason from that evidence and source.
Inspecting a MUX checkout does not prove that its revision produced the measured runtime.

Application `propose` entries can enter the existing exact-hash-approved app workflow.
Control/framework `recommend` entries use MUX-relative paths under `controls\dev` or `dxaml\xcp`
and remain `target-workflow-required` in the discovery report, not insufficient merely because the
app runner cannot execute them. The separate independent-trial workflow below supplies the native
coding/build/private-runtime path without rewriting that discovery report.
At any level, genuinely inadequate evidence remains `insufficient-evidence` with no file scope.
Legacy v1/v2 reports remain readable.

The additive `pack\knowledge\intent-catalog.md` helps translate any app/control request into
one falsifiable question, setup, actions, actual step count/waits, validity checks, useful
metrics, healthy result and regressions. New plans freeze that validated specification with
**one run per baseline/candidate**, not three repeats. Existing scenario planning and all
multidimensional analysis remain in place. Apps/control names are inputs, never built-in flows;
presented frames, allocation stacks or owned bytes are not asserted when unsupported.

`Analyze-Control.py` adds per-instance native timeline evidence and capture-local subtree
inventory for an explicitly supplied control type. Proven template spans are nested elapsed
time, not exclusive CPU; process working/private/commit and shared graphics remain distinct
from control-owned bytes. Unknown identities and ownership stay unknown. Optional
`-TargetControl <type>` selects this lens when reanalyzing legacy captures; new plans discover
the target in their intent specification. No alternate profiler is used.

An existing valid v3 investigation can receive a new three-level expert report without
rebuilding, recording or editing the app:

```powershell
.\scripts\Start-Investigation.ps1 -Phase Investigate `
    -Directory C:\PerfRuns\gallery-layers `
    -SourceInvestigation C:\PerfRuns\gallery-treeview `
    -Python C:\Tools\ui-profiler-mcp\Scripts\python.exe `
    -GitHubAccount <existing-licensed-account>
.\scripts\Start-Investigation.ps1 -Phase Report -Directory C:\PerfRuns\gallery-layers
```

Use a fresh output directory. This read-only investigation verifies existing source/evidence
bindings, takes zero captures, authorizes no edits, and does not alter the sealed source context
or expand its approval/capture budget. Its context cannot run a candidate.

## Independent application and native trials

`scripts\Start-RecommendationTrials.ps1` prepares fresh ADE workstreams/proposals from a saved
three-level read-only investigation. Each selected recommendation gets its own coding agent,
working copy, exact file scope and exact-hash approval; fixes are never combined.
Preparation does not approve, code, build a native runtime, or record.

```powershell
.\scripts\Start-RecommendationTrials.ps1 -Phase Prepare `
    -Directory C:\PerfRuns\nav-trials -SourceInvestigation C:\PerfRuns\gallery-layers `
    -RecommendationId @('application-recommendation-id', 'control-recommendation-id') `
    -GitHubAccount <existing-licensed-account>
# Review each proposal, then approve its exact hash using scripts\ade.ps1 approve.
.\scripts\Start-RecommendationTrials.ps1 -Phase Run -Directory C:\PerfRuns\nav-trials `
    -RecommendationId @('application-recommendation-id', 'control-recommendation-id')
.\scripts\Start-RecommendationTrials.ps1 -Phase Report -Directory C:\PerfRuns\nav-trials
```

Application trials reuse the original runtime/baseline. A native trial keeps the application
source/executable unchanged and builds baseline and candidate from independent native source
copies. Its proposal explicitly declares **one new source-matched native baseline and one
candidate capture**; this is a new bounded authorization, never a reset of the old investigation.
The original recording remains discovery evidence, not a falsely source-matched native baseline.

Native checks use the repository's fixed `initrun.ps1` build targets, privately stage the built
runtime closure and verify the actual loaded private runtime before and after the scenario.
They never globally register a runtime or silently fall back to the installed WinUI binary.
Visual Studio/SDK prerequisites must be available. A fresh checkout reporting the specific
missing-initialization error gets one logged `init.ps1 amd64fre` before a build retry; other build
failures do not retry. Missing or incompatible native output is an explicit failure. Captures run sequentially under the
trial-set operation lock. Failed or reserved captures and completed coding trials cannot retry.
Reports retain every level, including insufficient-evidence findings, failures and pending approvals.
Local reports always include actionable tasks, actual applied fixes when verified, and their
evidence or explicit missing/blocked status. These task rows do not create external work items.
**Azure DevOps task creation and GitHub draft PR creation are independent opt-ins, both off by
default.** Creating external tasks requires a separate explicit request; ordinary analysis,
preparation, execution and reporting do not create them. Existing externally created tasks
are not required to obtain the local report.
With explicit `-PublishDraftPullRequests`, only a verified measured fix with an observed primary
improvement may create a draft PR. The PR contains measured baseline/candidate differences,
regressions, unavailable dimensions, exact check commands/results and evidence provenance.
No-improvement, unapproved and failed trials retain honest local reports without fabricated links.
One pair is an observation, not statistical significance.

## Agents, MCPs and contracts

The controller calls `ui-profiler-harness` planning/build/UI/evidence/proposal tools.
The actual WinUI expert must query the unchanged **`winui-profiler` MCP** for original
recording evidence and the plugin-owned **`ui-profiler-evidence` MCP** for bounded
capability/metric/detail queries over hash-verified derived reports.
The two read-only servers are declared in each generated trial's `pack\mcp.json` for ADE;
the mutating outer harness server is not granted to the coding session.
The existing native MCP is privately published from unchanged tracked MUX source and
handshake/catalog-checked before a baseline recording. Dependency restore/build outputs stay
in the investigation; the original profiler source and lock files are not edited.
PowerShell scripts implement trusted, deterministic tools and checks; they do not replace
the planning agent, expert or real ADE coding executor.

[`CONTRACTS.md`](CONTRACTS.md) is active guidance: `harness_contracts` exposes it to the
controller, preparation freezes it into approved pack knowledge, and the actual expert
receives it. Its hash is checked before later tool operations/measurement.
Only legal pre-freeze replanning is allowed; models cannot reset budgets, rewrite frozen
inputs, relax protected-file/check policies or approve themselves to bypass a failure.
Executable guards and contract tests remain authoritative, not English-language interpretation.

The offline suite includes analysis/plan/MCP integrity tests and actual ADE boundary tests
using explicitly labeled inert script adapters in synthetic repositories. These are **harness
contract tests**, not AI runs, app launches or evidence that a generated fix is faster.
Live UI/profiling validation is separate and requires explicit authorization.

## Multidimensional evidence

Each trial's `multidimensional\comparison.md` summarizes its result; `comparison.json` and
`comparison.csv` contain every absolute/percentage difference and availability state.
Each side has `analysis.json`/`analysis.md`, canonical profiler query exports, and
an immutable `manifest.json`. Prepare supplies the baseline analysis to the real expert.

| Evidence | Compared dimensions |
|---|---|
| Scenario and UI ticks | Whole scenario and every action; elapsed ticks, p95/max/mean, drawn/slow counts and cadence |
| XAML/event activity | All retained provider/event counts, nested layout/render spans, navigation, focus and canonical regions |
| CPU | Native utilization; sampled modules/functions/stacks, UI vs all threads, symbol coverage and separate CPU stack drops |
| Process memory | Working set, private working set and private commit; point-sample distributions, endpoints, changes and gaps |
| GPU | Busiest-engine utilization; event-driven usage/budget carry-in and changes, separately by adapter ordinal and memory segment |
| Graphics/textures | Observed lifetimes, creation/destruction/unknown endpoints, descriptors, links and exact known allocation bytes |
| Objects/trees/resources | Indexed activity, captured properties/correlations, process-aware history matches, within-process tree diffs and supported runtime counters |
| Launch | First drawn XAML ticks and all reached/unreached profiler milestones |

The exporter uses MUX's existing `ReadTextureSnapshots` engine rather than guessing
texture/allocation ownership or pixel-size bytes. Shared allocations are deduplicated.
Across recordings, compare aggregates, never handles, node IDs or thread IDs.
GPU present/display-frame timing, heap-object ownership, texture pixels and a complete
XAML-to-texture ownership chain are explicitly **unsupported**, not zero.
Unsupported resource diagnostics, missing samples, unknown allocation sizes and
CPU stack drops remain visible. Activity/structural counts are context, not speedups.
Working set is residency; private commit is not. GPU utilization is not summed across engines.

New captures request an end tree/resource snapshot over the **same live connection**,
after the timed actions. This adds neither a recording nor an app dependency. Capture v2
hashes native NDJSON, snapshot and query evidence alongside SQLite/actions.
The Python analyzer uses only its standard library; the engine exporter needs PowerShell
with a .NET runtime compatible with the staged profiler (the current build uses .NET 10).

Existing recordings can be reanalyzed without a build, app launch, agent call or capture:

```powershell
.\pack\tools\Compare-Evidence.ps1 `
    -BaselinePath C:\PerfRuns\run\baseline\measurement.json `
    -CandidatePath C:\PerfRuns\run\candidate\measurement.json `
    -ProfilerCli C:\Tools\WinUIProfiler\WinUIProfiler.Cli.exe `
    -Directory C:\PerfRuns\run\multidimensional
```

Use a new output directory; completed reports are verified, not overwritten.
Legacy v1 recordings retain their original files and approval/provenance. Secondary
files are first sealed at reanalysis time, clearly labeled; missing end snapshots are
not manufactured. Changed source or evidence invalidates a cached analysis.

UI ticks are elapsed time, not CPU time or displayed frames; scenario wall time includes
automation waits. Nested spans are not additive CPU time. Partial CPU sampling cannot
establish a complete source cost. Passing verifies behavior and evidence, not a universal
speedup. Report regressions honestly: one pair is an observation, not statistical significance.
An agent must inspect actual initial UI state and preserve the user's scenario; it cannot assume
Gallery's fixture describes another app. All content and accessibility must remain functional
after a proposed fix. One unrecorded preview, one shared baseline and one capture per approved recommendation are bounded operations;
this is permission/state/integrity enforcement on the engineer's machine, **not an OS sandbox**.
