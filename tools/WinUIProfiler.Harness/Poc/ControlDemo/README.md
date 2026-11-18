# Real WinUIProfiler control POC

This second POC connects the ADE engine to a real WinUI app and the in-repository
WinUIProfiler. It is local-only and intentionally small.

## What it measures

The app inserts 40 Buttons into a StackPanel. The seed deliberately forces
layout after each insertion. A protected caller performs one final layout and
verifies every button's count, order, and content. ETW markers bound only that
scenario, not app launch.

`capture.ps1` builds the sample once per measurement and launches ten independent
app processes. It owns a unique logman session and output directory for each
iteration, stops its session, and calls the real WinUIProfiler CLI:

```text
WinUIProfiler.Cli.exe record <trace.etl> --pid <owned-pid> --out <session.sqlite> --markers <markers.json>
```

`metrics.py` opens each SQLite session read-only. It requires the expected process,
both UI-thread markers, a complete untrimmed import, zero reported event loss,
and balanced XAML Layout/Start (41) / Layout/Stop (42) pairs from the XAML provider
inside those markers. Missing events are invalid data, never a measured zero.

`verify.py` returns 0 only when all ten valid iterations have at most four layout
passes and the app's correctness assertions pass. Exit 1 means measured work
exceeds the budget; exit 2 means capture/import/environment failure. Per-run and
mean values, ETLs, databases, source hashes, and runtime identity are retained.

Builds use unique folders under `%LOCALAPPDATA%\WinUIProfilerHarness\Builds`.
Their executable paths are recorded in the capture manifest. Keeping this path
short avoids the XAML compiler metadata loader's legacy MAX_PATH limit.

## Running it through ADE

Use the parent `Invoke-AdePoc.ps1` with `-Action Prepare -Scenario Control`.
It creates the machine-specific `measurement-contract.json` only in the external
staging folder, preserves the failing seed here, and selects `control-pack`
instead of the unrelated synthetic catalog pack. Subsequent actions use the
returned explicit state path. See the parent README for the complete commands.

Only `ScenarioWorkload.cs` is editable during the ADE run. Obtain human approval
of the new proposal hash before approving or executing it. The original seed
and framework source remain unchanged.

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

Extractor tests use explicitly synthetic SQLite fixtures, not claimed captures:

```powershell
python -B -m unittest discover -s tests -t .
```
