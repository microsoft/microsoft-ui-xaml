# Real WinUIProfiler control POC

This second POC connects the ADE engine to a real WinUI app and the in-repository
WinUIProfiler. It is local-only and intentionally small.

## What it measures

By default the app inserts 40 Buttons into a StackPanel. The seed deliberately forces
layout after each insertion. A protected caller performs one final layout and
verifies every button's count, order, and content. ETW markers bound only that
scenario, not app launch.

The shared `..\helpers\capture.ps1` builds the sample once per measurement and launches ten independent
app processes. It owns a unique logman session and output directory for each
iteration, stops its session, and calls the real WinUIProfiler CLI:

```text
WinUIProfiler.Cli.exe record <trace.etl> --pid <owned-pid> --out <session.sqlite> --markers <markers.json>
```

The shared `..\helpers\metrics.py` opens each SQLite session read-only. It requires the expected process,
both UI-thread markers, a complete untrimmed import, zero reported event loss,
and balanced XAML Layout/Start (41) / Layout/Stop (42) pairs from the XAML provider
inside those markers. Missing layout telemetry is invalid in the default scenario.

For the default scenario, the shared `..\helpers\verify.py` returns 0 only when all ten valid iterations have at most four layout
passes and the app's correctness assertions pass. Exit 1 means measured work
exceeds the budget; exit 2 means capture/import/environment failure. Per-run and
mean values, ETLs, databases, source hashes, and runtime identity are retained.

Builds use unique folders under `%LOCALAPPDATA%\WinUIProfilerHarness\Builds`.
Their executable paths are recorded in the capture manifest. Keeping this path
short avoids the XAML compiler metadata loader's legacy MAX_PATH limit.
New captures retain the build log and both executable and managed DLL hashes.

Only app-specific inputs live here: the app/project, `build.ps1`, provider/marker
files, and `scenario.json`. The scenario contract owns the 40-button expectation,
marker payloads, layout budget, source snapshots, and allowed edit paths. Capture,
SQLite extraction, profiler queries, and verification live in the parent
`helpers` directory; their tests live in the parent `tests` directory.
The app also owns `ScenarioPlan.cs`, `scenario-capabilities.json` and
`scenario_plan.py`: these define and validate its bounded action surface, not
shared capture infrastructure.

## User-selected flows

Describe the flow in Copilot chat. The `winui-profiler-scenario` repository skill
asks for missing details, retains explicit clarifications, validates a bounded
JSON plan behind the scenes, and shows readable steps for confirmation.
The user does not write JSON or copy hashes. Planning never builds or runs
the app. After confirmation Copilot calls the existing adapter with the reviewed
plan and its hash. See the parent README for a conversation example.

The initial supported actions replace the panel with 1-200 buttons, clear it,
resize the window (physical pixels), or scroll to the start/end. At most 12
steps execute in order through app-owned hooks, with bounded settling and
per-step correctness. This is not physical-input automation or arbitrary test
generation. Unsupported or incomplete requests do not fall back to the seed.

A custom flow starts after window load; explicitly requested prepopulation is
unmeasured setup. Each measured run records its frozen plan hash, executed
step count, final tree and per-step counts/dimensions/scroll offsets in
`app-result.json`, plus ETW step markers. Capture rejects a different plan or
incorrect result. The exact plan is retained with source hashes.

Ten valid custom runs return **Measured** and exit 0, not a passed performance
budget. Custom flows may use the separately approved investigation workflow;
the existing seeded fix gate is blocked for them. A zero-layout interval is
valid only with explicit opt-in and positive layout-provider evidence from the
same process/UI thread elsewhere in the complete trace. Unbalanced events,
missing providers and reported event loss still fail.

## Running it through ADE

Use the parent `Invoke-AdePoc.ps1` with `-Action Prepare -Scenario Control`.
It creates the machine-specific `measurement-contract.json` only in the external
staging folder, preserves the failing seed here, and selects `control-pack`
instead of the unrelated synthetic catalog pack. Subsequent actions use the
returned explicit state path. See the parent README for the complete commands.

Only `ScenarioWorkload.cs` is editable during the ADE run. Obtain human approval
of the new proposal hash before approving or executing it. The original seed
and framework source remain unchanged.

The parent adapter also supports `Investigate`: a separately approved ADE
research run queries the profiler, records observations before reading app
source, selects a drilldown, and writes a diagnosis without changing source.
It receives a measured source snapshot, not this README's description of the
seeded issue. A subsequent fix proposal consumes the verified diagnosis.
See the parent walkthrough for the separate approval gates and evidence checks.

The app normally closes immediately. `Show -DisplaySeconds 30` on a newly
prepared state keeps it visible after its scenario markers, records one
observation capture, and closes it automatically. This is not a ten-run
acceptance check. `Evidence -CandidateManifest <path>` independently recounts
the saved baseline and candidate SQLite sessions; it does not trust a model's
summary of the counts.
`Explore -Scenario Control` instead opens the default app without recording
anything and waits until its window is closed; no prepared state is needed.

## Prerequisites and boundaries

- Windows x64, an interactive desktop, .NET 10 SDK, Python, PowerShell 7, and the
  existing ADE prerequisites. Build the in-repository profiler CLI first.
- The public WinUI package and transitive dependencies are locked. This demo
  uses that self-contained runtime, not a locally rebuilt framework.
- ETW capture needs suitable permissions, such as enabled Performance Log Users
  membership or elevation. The runner does not elevate or alter group membership.
- Only the owned app and ETW session are stopped. Do not run simultaneous local
  benchmarks; unrelated system load can affect timing.
- Raw ETLs are provider-wide and can include other local apps. Only the demo PID
  is imported, and no trace or SQLite file should be uploaded.
- The initial capture backend is **logman**, not WPR: `.wprp`, VM deployment,
  TestGenerator, live resource inspection, and the three MAF agents are deferred.
- Lower measured layout count demonstrates reduced framework work from fixing a
  deliberately constructed **app-level** issue. Durations are informational,
  not a statistical acceptance gate or evidence of a framework bug.
- ADE runs commands as the engineer and is **not a security sandbox**.

Shared helper tests use explicitly synthetic SQLite fixtures, not claimed captures.
Run from the POC directory:

```powershell
python -B -m unittest discover -s tests -t .
```
