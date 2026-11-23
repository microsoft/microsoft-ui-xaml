# ui-profiler ADE team plugin

ADE is an external prerequisite, not copied into the app or MUX. Generate/install the
shared runtime with the ADE platform's `setup.ps1`. This directory is the team plugin.
It uses only MUX WinUIProfiler; it does not use an alternative profiling backend.

The current minimal adapter supports clean Git repositories containing unpackaged,
self-contained .NET WinUI apps. An external scenario supplies the project, executable,
window size, selectors, and UI actions. Gallery is only the supplied example.
The harness owns build/deployment and external UI Automation; the app has no harness references.
Use an interactive Windows desktop. Keyboard actions refuse to run if the owned app loses focus.
Choose a short investigation directory: WinUI's generated-code paths can exceed legacy path limits.

```powershell
.\scripts\Start-Investigation.ps1 -Phase Prepare `
    -Directory C:\PerfRuns\gallery-treeview `
    -ApplicationRepository Q:\Repo\WinUI-Gallery `
    -ScenarioPath .\scenarios\gallery-treeview.json `
    -KnowledgeRepository Q:\Repo\microsoft-ui-xaml `
    -ExpertHome C:\Tools\WinUI_Expert_Agent `
    -ProfilerCli C:\Tools\WinUIProfiler\WinUIProfiler.Cli.exe `
    -Dotnet C:\Tools\dotnet\dotnet.exe `
    -AdeCopilotCli C:\Tools\Copilot\copilot.exe `
    -GitHubAccount <existing-licensed-account>
```

Build the CLI with MUX's existing `tools\WinUIProfiler\scripts\Build-Cli.ps1`; distribute
its entire output, not just the executable. Supply the actual Godly.Tools
`WinUI_Expert_Agent` bundle. Local RAG indexes its existing KB; no collectors,
global agent installation, scheduled tasks, or automatic refresh are used.
The selected existing GitHub credential is scoped to the harness's child processes, not a global account switch.
ADE uses its own task-local client state and the same explicitly selected account.
`-AdeCopilotCli` pins a compatible native CLI without changing the machine install;
Prepare checks SDK/server compatibility and authentication before spending a capture.

Prepare copies application source, publishes a release unpackaged deployment,
stages the complete evaluated build-output closure (including app PRI/compiled XAML),
records the scenario once, validates/hashes SQLite evidence, obtains the actual
read-only expert recommendation from the full dimensional analysis, and creates an ADE proposal/workstream.
Review its plan and exact proposal hash. **The harness never approves for you.**

```powershell
.\scripts\ade.ps1 approve --run <runId> --hash <proposalSha256> `
    --runs-dir C:\PerfRuns\gallery-treeview\ade-runs
.\scripts\Start-Investigation.ps1 -Phase Run -Directory C:\PerfRuns\gallery-treeview `
    -GitHubAccount <existing-licensed-account>
.\scripts\Start-Investigation.ps1 -Phase Verify -Directory C:\PerfRuns\gallery-treeview
.\tests\Test-Harness.ps1
```

ADE owns the real Copilot coding executor, proposal approval, isolated working copy,
single coding round, verification and provenance. Shell/URL access is disabled for
the coding agent. Its check builds/deploys the candidate and runs the same scenario once.
Source-matched cached checks do not consume another capture. A failed or reserved
capture cannot automatically retry. Do not apply the ADE diff to the original app
unless separately requested.

`multidimensional\comparison.md` summarizes the result; `comparison.json` and
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

Future captures request an end tree/resource snapshot over the **same live connection**,
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
The example includes a preparatory collapse because Gallery's basic tree starts expanded.
All sample content and accessibility must remain functional after a proposed fix.
